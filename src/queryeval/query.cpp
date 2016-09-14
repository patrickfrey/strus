/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "query.hpp"
#include "queryEval.hpp"
#include "accumulator.hpp"
#include "ranker.hpp"
#include "keyMap.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "strus/summaryElement.hpp"
#include "docsetPostingIterator.hpp"
#include "private/utils.hpp"
#include "strus/base/snprintf.h"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <cstdio>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

///\brief Constructor
Query::Query( const QueryEval* queryEval_, const StorageClientInterface* storage_, ErrorBufferInterface* errorhnd_)
	:m_queryEval(queryEval_)
	,m_storage(storage_)
	,m_metaDataReader(storage_->createMetaDataReader())
	,m_metaDataRestriction()
	,m_nofRanks(20)
	,m_minRank(0)
	,m_evalset_defined(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_metaDataReader.get()) throw strus::runtime_error(_TXT("error creating meta data reader"));
	const ScalarFunctionInterface* weightingFormula = m_queryEval->weightingFormula();
	if (weightingFormula)
	{
		m_weightingFormula.reset( weightingFormula->createInstance());
	}

	std::vector<TermConfig>::const_iterator
		ti = m_queryEval->terms().begin(),
		te = m_queryEval->terms().end();
	for (; ti != te; ++ti)
	{
		pushTerm( ti->type, ti->value);
		defineFeature( ti->set, 1.0);
	}
}

bool Query::Term::operator<( const Term& o) const
{
	int cmpres;
	if (type.size() != o.type.size()) return type.size() < o.type.size();
	if ((cmpres = std::strcmp( type.c_str(), o.type.c_str())) < 0) return cmpres;
	if (value.size() != o.value.size()) return value.size() < o.value.size();
	if ((cmpres = std::strcmp( value.c_str(), o.value.c_str())) < 0) return cmpres;
	return 0;
}

void Query::printStack( std::ostream& out, std::size_t indent) const
{
	std::vector<NodeAddress>::const_iterator si = m_stack.begin(), se = m_stack.end();
	std::size_t sidx = m_stack.size();
	out << std::string( indent*2, ' ') << "Stack" << std::endl;
	for (--sidx; si != se; ++si,--sidx)
	{
		out << "STK [" << sidx << "] ";
		printNode( out, *si, indent+1);
	}
}

void Query::pushTerm( const std::string& type_, const std::string& value_)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	char buf[ 2048];
	strus_snprintf( buf, sizeof(buf), "pushTerm %s %s stack %u\n", type_.c_str(), value_.c_str(), (unsigned int)m_stack.size());
	std::cerr << buf;
	printStack( std::cerr, 1);
#endif
	try
	{
		m_terms.push_back( Term( type_, value_));
		m_stack.push_back( nodeAddress( TermNode, m_terms.size()-1));
	}
	CATCH_ERROR_MAP( _TXT("error pushing term to query: %s"), *m_errorhnd);
}

void Query::pushExpression( const PostingJoinOperatorInterface* operation, std::size_t argc, int range_, unsigned int cardinality_)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	char buf[ 2048];
	strus_snprintf( buf, sizeof(buf), "pushExpression %u stack %u\n", (unsigned int)argc, (unsigned int)m_stack.size());
	std::cerr << buf;
	printStack( std::cerr, 1);
#endif
	try
	{
		if (argc > m_stack.size())
		{
			m_errorhnd->report( _TXT( "illegal expression definition: size of expression bigger than stack size"));
		}
		else
		{
			std::vector<NodeAddress> subnodes;
			std::vector<NodeAddress>::const_iterator si = m_stack.end() - argc, se = m_stack.end();
			for (; si != se; ++si)
			{
				subnodes.push_back( *si);
			}
			m_expressions.push_back( Expression( operation, subnodes, range_, cardinality_));
			m_stack.resize( m_stack.size() - argc);
			m_stack.push_back( nodeAddress( ExpressionNode, m_expressions.size()-1));
		}
	}
	CATCH_ERROR_MAP( _TXT("error pushing expression to query: %s"), *m_errorhnd);
}

void Query::attachVariable( const std::string& name_)
{
	try
	{
		if (m_stack.empty())
		{
			m_errorhnd->report( _TXT( "cannot attach variable (query stack empty)"));
		}
		m_variableAssignments.insert( std::pair<NodeAddress,std::string>( m_stack.back(), name_));
	}
	CATCH_ERROR_MAP( _TXT("error attaching variables to query: %s"), *m_errorhnd);
}

void Query::defineFeature( const std::string& set_, double weight_)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	char buf[ 2048];
	strus_snprintf( buf, sizeof(buf), "defineFeature %s stack %u\n", set_.c_str(), (unsigned int)m_stack.size());
	std::cerr << buf;
	printStack( std::cerr, 1);
#endif
	try
	{
		m_features.push_back( Feature( utils::tolower(set_), m_stack.back(), weight_));
		m_stack.pop_back();
	}
	CATCH_ERROR_MAP( _TXT("error define feature of query: %s"), *m_errorhnd);
}

void Query::addMetaDataRestrictionCondition(
		MetaDataRestrictionInterface::CompareOperator opr, const std::string&  name,
		const NumericVariant& operand, bool newGroup)
{
	try
	{
		if (!m_metaDataRestriction.get())
		{
			m_metaDataRestriction.reset( m_storage->createMetaDataRestriction());
		}
		m_metaDataRestriction->addCondition( opr, name, operand, newGroup);
	}
	CATCH_ERROR_MAP( _TXT("error define meta data restriction of query: %s"), *m_errorhnd);
}

void Query::addDocumentEvaluationSet(
		const std::vector<Index>& docnolist_)
{
	try
	{
		m_evalset_docnolist.insert( m_evalset_docnolist.end(), docnolist_.begin(), docnolist_.end());
		std::sort( m_evalset_docnolist.begin(), m_evalset_docnolist.end());
		m_evalset_defined = true;
	}
	CATCH_ERROR_MAP( _TXT("error define document evaluation set of query: %s"), *m_errorhnd);
}

void Query::print( std::ostream& out) const
{
	out << "query evaluation program:" << std::endl;
	m_queryEval->print( out);

	std::vector<Feature>::const_iterator fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		char buf[ 128];
		strus_snprintf( buf, sizeof(buf), "%.5f", fi->weight);
		out << "feature '" << fi->set << "' " << buf << ": " << std::endl;
		printNode( out, fi->node, 1);
		out << std::endl;
	}
}

void Query::printVariables( std::ostream& out, NodeAddress adr) const
{
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> variables = m_variableAssignments.equal_range( adr);

	if (variables.first != variables.second)
	{
		out << " [";
		Itr vi = variables.first, ve = variables.second;
		for (std::size_t vidx=0; vi != ve; ++vi,++vidx)
		{
			if (vidx) out << ",";
			out << vi->second;
		}
		out << "]";
	}
}

void Query::printNode( std::ostream& out, NodeAddress adr, std::size_t indent) const
{
	std::string indentstr( indent*2, ' ');
	switch (nodeType( adr))
	{
		case NullNode:
			out << indentstr << "NULL";
			printVariables( out, adr);
			out << std::endl;
			break;
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( adr)];
			out << indentstr << "term " << term.type << " '" << term.value << "'";
			printVariables( out, adr);
			out << std::endl;
			break;
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			out << indentstr << std::hex<< (uintptr_t)expr.operation << std::dec << " " << expr.range << ":";
			printVariables( out, adr);
			out << std::endl;
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(),
				ne = expr.subnodes.end();
			for (; ni != ne; ++ni)
			{
				printNode( out, *ni, indent+1);
			}
			break;
		}
	}
}

Query::NodeAddress Query::duplicateNode( Query::NodeAddress adr)
{
	Query::NodeAddress rtadr = nodeAddress( NullNode, 0);
	switch (nodeType( adr))
	{
		case NullNode:
			rtadr = nodeAddress( NullNode, 0);
			break;
		case TermNode:
		{
			m_terms.push_back( m_terms[ nodeIndex( adr)]);
			rtadr = nodeAddress( TermNode, m_terms.size()-1);
			break;
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			std::vector<NodeAddress> subnodes;
			std::vector<NodeAddress>::const_iterator si = expr.subnodes.begin(), se = expr.subnodes.end();
			for (; si != se; ++si)
			{
				subnodes.push_back( duplicateNode( *si));
			}
			m_expressions.push_back( Expression( expr.operation, subnodes, expr.range, expr.cardinality));
			rtadr = nodeAddress( ExpressionNode, m_expressions.size()-1);
			break;
		}
	}
	// Duplicate assigned variables too:
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> vrange = m_variableAssignments.equal_range( adr);

	Itr vi = vrange.first, ve = vrange.second;
	for (; vi != ve; ++vi)
	{
		m_variableAssignments.insert( std::pair<NodeAddress,std::string>( rtadr, vi->second));
	}
	// Return result:
	return rtadr;
}

void Query::setMaxNofRanks( std::size_t nofRanks_)
{
	m_nofRanks = nofRanks_;
}

void Query::setMinRank( std::size_t minRank_)
{
	m_minRank = minRank_;
}

void Query::addUserName( const std::string& username_)
{
	try
	{
		m_usernames.push_back( username_);
	}
	CATCH_ERROR_MAP( _TXT("error adding user to query: %s"), *m_errorhnd);
}

PostingIteratorInterface* Query::createExpressionPostingIterator( const Expression& expr, NodeStorageDataMap& nodeStorageDataMap)
{
	enum {MaxNofJoinopArguments=256};
	if (expr.subnodes.size() > MaxNofJoinopArguments)
	{
		throw strus::runtime_error( _TXT( "number of arguments of feature join expression in query out of range"));
	}
	std::vector<Reference<PostingIteratorInterface> > joinargs;
	std::vector<NodeAddress>::const_iterator
		ni = expr.subnodes.begin(), ne = expr.subnodes.end();
	for (; ni != ne; ++ni)
	{
		switch (nodeType( *ni))
		{
			case NullNode:
				break;
			case TermNode:
			{
				const Term& term = m_terms[ nodeIndex( *ni)];
				joinargs.push_back( m_storage->createTermPostingIterator( term.type, term.value));
				if (!joinargs.back().get()) throw strus::runtime_error(_TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = NodeStorageData( joinargs.back().get(), getTermStatistics( term.type, term.value));
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ nodeIndex(*ni)], nodeStorageDataMap));
				if (!joinargs.back().get()) throw strus::runtime_error(_TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = NodeStorageData( joinargs.back().get());
				break;
		}
	}
	return expr.operation->createResultIterator( joinargs, expr.range, expr.cardinality);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& nodeadr, NodeStorageDataMap& nodeStorageDataMap)
{
	PostingIteratorInterface* rt = 0;
	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Term& term = m_terms[ nidx];
			rt = m_storage->createTermPostingIterator( term.type, term.value);
			if (!rt) break;
			nodeStorageDataMap[ nodeadr] = NodeStorageData( rt, getTermStatistics( term.type, term.value));
			break;
		}
		case ExpressionNode:
			std::size_t nidx = nodeIndex( nodeadr);
			rt = createExpressionPostingIterator( m_expressions[ nidx], nodeStorageDataMap);
			if (!rt) break;
			nodeStorageDataMap[ nodeadr] = NodeStorageData( rt);
			break;
	}
	return rt;
}

const Query::NodeStorageData& Query::nodeStorageData( const NodeAddress& nodeadr, const NodeStorageDataMap& nodeStorageDataMap) const
{
	NodeStorageDataMap::const_iterator pi = nodeStorageDataMap.find( nodeadr);
	if (pi == nodeStorageDataMap.end())
	{
		throw strus::runtime_error( _TXT( "expression node postings not found"));
	}
	return pi->second;
}

void Query::collectSummarizationVariables(
			std::vector<SummarizationVariable>& variables,
			const NodeAddress& nodeadr,
			const NodeStorageDataMap& nodeStorageDataMap)
{
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> vrange = m_variableAssignments.equal_range( nodeadr);

	Itr vi = vrange.first, ve = vrange.second;
	for (; vi != ve; ++vi)
	{
		const NodeStorageData& nd = nodeStorageData( nodeadr, nodeStorageDataMap);
		variables.push_back( SummarizationVariable( vi->second, nd.itr));
	}

	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode: break;
		case ExpressionNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Expression& expr = m_expressions[ nidx];
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(), 
				ne = expr.subnodes.end();
			for (; ni != ne; ++ni)
			{
				collectSummarizationVariables( variables, *ni, nodeStorageDataMap);
			}
			break;
		}
	}
}

void Query::defineTermStatistics(
		const std::string& type_,
		const std::string& value_,
		const TermStatistics& stats_)
{
	m_termstatsmap[ Term( type_, value_)] = stats_;
}

void Query::defineGlobalStatistics(
		const GlobalStatistics& stats_)
{
	m_globstats = stats_;
}

const TermStatistics& Query::getTermStatistics( const std::string& type_, const std::string& value_) const
{
	static TermStatistics undef;
	if (m_termstatsmap.empty()) return undef;
	std::map<Term,TermStatistics>::const_iterator si = m_termstatsmap.find( Term( type_, value_));
	if (si == m_termstatsmap.end()) return undef;
	return si->second;
}

void Query::setWeightingVariableValue(
		const std::string& name, double value)
{
	if (!m_weightingFormula.get())
	{
		m_errorhnd->report(_TXT("try to defined weighting variable without weighting formula defined"));
	}
	else
	{
		m_weightingFormula->setVariableValue( name, value);
	}
}

QueryResult Query::evaluate()
{
	const char* evaluationPhase = "query feature postings initialization";
	try
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "evaluate query:" << std::endl;
		print( std::cout);
#endif
		// [1] Check initial conditions:
		if (m_nofRanks == 0)
		{
			return QueryResult();
		}
		if (m_queryEval->weightingFunctions().empty())
		{
			m_errorhnd->report( _TXT( "cannot evaluate query, no weighting function defined"));
			return QueryResult();
		}
		if (m_queryEval->selectionSets().empty())
		{
			m_errorhnd->report( _TXT( "cannot evaluate query, no selection features defined"));
			return QueryResult();
		}
		NodeStorageDataMap nodeStorageDataMap;

		// [3] Create the posting sets of the query features:
		std::vector<Reference<PostingIteratorInterface> > postings;
		{
			std::vector<Feature>::const_iterator
				fi = m_features.begin(), fe = m_features.end();
			for (; fi != fe; ++fi)
			{
				Reference<PostingIteratorInterface> postingsElem(
					createNodePostingIterator( fi->node, nodeStorageDataMap));
				if (!postingsElem.get()) return QueryResult();
				postings.push_back( postingsElem);
			}
		}
		// [4] Create the accumulator:
		DocsetPostingIterator evalset_itr;
		Accumulator accumulator(
			m_storage,
			m_metaDataReader.get(), m_metaDataRestriction.get(), m_weightingFormula.get(),
			m_minRank + m_nofRanks, m_storage->maxDocumentNumber());

		// [4.1] Define document subset to evaluate query on:
		if (m_evalset_defined)
		{
			evalset_itr = DocsetPostingIterator( m_evalset_docnolist);
			accumulator.defineEvaluationSet( &evalset_itr);
		}
		// [4.2] Add document selection postings:
		{
			std::vector<std::string>::const_iterator
				si = m_queryEval->selectionSets().begin(),
				se = m_queryEval->selectionSets().end();
	
			for (int sidx=0; si != se; ++si,++sidx)
			{
				std::vector<Feature>::const_iterator
					fi = m_features.begin(), fe = m_features.end();
				for (; fi != fe; ++fi)
				{
					if (*si == fi->set)
					{
						accumulator.addSelector(
							nodeStorageData( fi->node, nodeStorageDataMap).itr, sidx);
					}
				}
			}
		}
		evaluationPhase = "weighting functions initialization";
		// [4.3] Add features for weighting:
		{
			std::vector<WeightingDef>::const_iterator
				wi = m_queryEval->weightingFunctions().begin(),
				we = m_queryEval->weightingFunctions().end();
			for (; wi != we; ++wi)
			{
				std::auto_ptr<WeightingFunctionContextInterface> execContext(
					wi->function()->createFunctionContext(
						m_storage, m_metaDataReader.get(), m_globstats));
				if (!execContext.get()) throw strus::runtime_error(_TXT("error creating weighting function context"));
	
				std::vector<QueryEvalInterface::FeatureParameter>::const_iterator
					si = wi->featureParameters().begin(),
					se = wi->featureParameters().end();
				for (; si != se; ++si)
				{
					std::vector<Feature>::const_iterator
						fi = m_features.begin(), fe = m_features.end();
					for (; fi != fe; ++fi)
					{
						if (si->featureSet() == fi->set)
						{
							const NodeStorageData& nd = nodeStorageData( fi->node, nodeStorageDataMap);
							execContext->addWeightingFeature(
								si->parameterName(), nd.itr, fi->weight, nd.stats);
#ifdef STRUS_LOWLEVEL_DEBUG
							std::cout << "add feature parameter " << si->parameterName() << "=" << fi->set << ' ' << fi->weight << std::endl;
#endif
						}
					}
				}
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "add feature " << wi->functionName() << std::endl;
#endif
				accumulator.addWeightingElement( execContext.release());
			}
		}
		evaluationPhase = "restrictions initialization";
		// [4.4] Define the user ACL restrictions:
		std::vector<Reference<InvAclIteratorInterface> > invAclList;
		std::vector<std::string>::const_iterator ui = m_usernames.begin(), ue = m_usernames.end();
		for (; ui != ue; ++ui)
		{
			Reference<InvAclIteratorInterface> invAcl( m_storage->createInvAclIterator( *ui));
			if (invAcl.get())
			{
				invAclList.push_back( invAcl);
				accumulator.addAlternativeAclRestriction( invAcl.get());
			}
		}
		// [4.5] Define the feature restrictions:
		{
			std::vector<std::string>::const_iterator
				xi = m_queryEval->restrictionSets().begin(),
				xe = m_queryEval->restrictionSets().end();
			for (; xi != xe; ++xi)
			{
				std::vector<Feature>::const_iterator
					fi = m_features.begin(), fe = m_features.end();
				for (; fi != fe; ++fi)
				{
					if (*xi == fi->set)
					{
						accumulator.addFeatureRestriction(
							nodeStorageData( fi->node, nodeStorageDataMap).itr, false);
					}
				}
			}
		}
		// [4.6] Define the feature exclusions:
		{
			std::vector<std::string>::const_iterator
				xi = m_queryEval->exclusionSets().begin(),
				xe = m_queryEval->exclusionSets().end();
			for (; xi != xe; ++xi)
			{
				std::vector<Feature>::const_iterator
					fi = m_features.begin(), fe = m_features.end();
				for (; fi != fe; ++fi)
				{
					if (*xi == fi->set)
					{
						accumulator.addFeatureRestriction(
							nodeStorageData( fi->node, nodeStorageDataMap).itr, true);
					}
				}
			}
		}
		evaluationPhase = "document ranking";
		// [5] Do the ranking:
		std::vector<ResultDocument> ranks;
		Ranker ranker( m_nofRanks + m_minRank);
		Index docno = 0;
		unsigned int state = 0;
		unsigned int prev_state = 0;
		double weight = 0.0;
	
		while (accumulator.nextRank( docno, state, weight))
		{
			ranker.insert( WeightedDocument( docno, weight));
			if (state > prev_state && ranker.nofRanks() >= m_nofRanks + m_minRank)
			{
				state = prev_state;
				break;
			}
			prev_state = state;
		}
		std::vector<WeightedDocument>
			resultlist = ranker.result( m_minRank);
	
		evaluationPhase = "summarization";
		// [6] Create the summarizers:
		std::vector<Reference<SummarizerFunctionContextInterface> > summarizers;
		if (!resultlist.empty())
		{
			std::vector<SummarizerDef>::const_iterator
				zi = m_queryEval->summarizers().begin(),
				ze = m_queryEval->summarizers().end();
			for (; zi != ze; ++zi)
			{
				// [5.1] Create the summarizer:
				summarizers.push_back(
					zi->function()->createFunctionContext(
						m_storage, m_metaDataReader.get(), m_globstats));
				SummarizerFunctionContextInterface* closure = summarizers.back().get();
				if (!closure) throw strus::runtime_error(_TXT("error creating summarizer context"));

				// [5.2] Add features with their variables assigned to summarizer:
				std::vector<QueryEvalInterface::FeatureParameter>::const_iterator
					si = zi->featureParameters().begin(),
					se = zi->featureParameters().end();
				for (; si != se; ++si)
				{
					std::vector<Feature>::const_iterator
						fi = m_features.begin(), fe = m_features.end();
					for (; fi != fe; ++fi)
					{
						if (fi->set == si->featureSet())
						{
							std::vector<SummarizationVariable> variables;
							collectSummarizationVariables( variables, fi->node, nodeStorageDataMap);

							const NodeStorageData& nd = nodeStorageData( fi->node, nodeStorageDataMap);
							closure->addSummarizationFeature(
								si->parameterName(), nd.itr,
								variables, fi->weight, nd.stats);
						}
					}
				}
			}
		}
		evaluationPhase = "building of the result";
		// [7] Build the result:
		std::vector<WeightedDocument>::const_iterator ri=resultlist.begin(),re=resultlist.end();
		for (; ri != re; ++ri)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "result rank docno=" << ri->docno() << ", weight=" << ri->weight() << std::endl;
#endif
			std::vector<SummaryElement> summaries;

			std::vector<Reference<SummarizerFunctionContextInterface> >::iterator
				si = summarizers.begin(), se = summarizers.end();
			for (;si != se; ++si)
			{
				std::vector<SummaryElement> summary = (*si)->getSummary( ri->docno());
				summaries.insert( summaries.end(), summary.begin(), summary.end());
			}
			ranks.push_back( ResultDocument( *ri, summaries));
		}
		if (m_errorhnd->hasError())
		{
			throw strus::runtime_error( m_errorhnd->fetchError());
		}
		return QueryResult( state, accumulator.nofDocumentsRanked(), accumulator.nofDocumentsVisited(), ranks);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error during %s when evaluating query: %s"), evaluationPhase, *m_errorhnd, QueryResult());
}


