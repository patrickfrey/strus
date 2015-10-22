/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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
#include "docsetPostingIterator.hpp"
#include "private/utils.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>
#include <utility>
#include <algorithm>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

///\brief Constructor
Query::Query( const QueryEval* queryEval_, const StorageClientInterface* storage_, ErrorBufferInterface* errorhnd_)
	:m_queryEval(queryEval_)
	,m_storage(storage_)
	,m_metaDataReader(storage_->createMetaDataReader())
	,m_evalset_defined(false)
	,m_errorhnd(errorhnd_)
{
	if (!m_metaDataReader.get()) throw strus::runtime_error(_TXT("error creating meta data reader"));

	std::vector<TermConfig>::const_iterator
		ti = m_queryEval->terms().begin(),
		te = m_queryEval->terms().end();
	for (; ti != te; ++ti)
	{
		pushTerm( ti->type, ti->value);
		defineFeature( ti->set, 1.0);
	}
}

Query::Query( const Query& o)
	:m_queryEval(o.m_queryEval)
	,m_storage(o.m_storage)
	,m_metaDataReader(o.m_metaDataReader)
	,m_terms(o.m_terms)
	,m_expressions(o.m_expressions)
	,m_features(o.m_features)
	,m_stack(o.m_stack)
	,m_metaDataRestrictions(o.m_metaDataRestrictions)
	,m_variableAssignments(o.m_variableAssignments)
	,m_nofRanks(o.m_nofRanks)
	,m_minRank(o.m_minRank)
	,m_usernames(o.m_usernames)
	,m_evalset_docnolist(o.m_evalset_docnolist)
	,m_evalset_defined(o.m_evalset_defined)
	,m_errorhnd(o.m_errorhnd)
{}

void Query::pushTerm( const std::string& type_, const std::string& value_)
{
	try
	{
		m_terms.push_back( Term( type_, value_));
		m_stack.push_back( nodeAddress( TermNode, m_terms.size()-1));
	}
	CATCH_ERROR_MAP( _TXT("error pushing term to query: %s"), *m_errorhnd);
}

void Query::pushExpression( const PostingJoinOperatorInterface* operation, std::size_t argc, int range_, unsigned int cardinality_)
{
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

void Query::pushDuplicate()
{
	try
	{
		if (m_stack.empty())
		{
			throw strus::runtime_error( _TXT( "cannot push duplicate (query stack empty)"));
		}
		m_stack.push_back( duplicateNode( m_stack.back()));
	}
	CATCH_ERROR_MAP( _TXT("error pushing duplicate to query: %s"), *m_errorhnd);
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

void Query::defineFeature( const std::string& set_, float weight_)
{
	try
	{
		m_features.push_back( Feature( utils::tolower(set_), m_stack.back(), weight_));
		m_stack.pop_back();
	}
	CATCH_ERROR_MAP( _TXT("error define feature of query: %s"), *m_errorhnd);
}

void Query::defineMetaDataRestriction(
		CompareOperator opr, const std::string&  name,
		const ArithmeticVariant& operand, bool newGroup)
{
	try
	{
		Index hnd = m_metaDataReader->elementHandle( name);
		const char* typeName = m_metaDataReader->getType( hnd);
		m_metaDataRestrictions.push_back( MetaDataRestriction( typeName, opr, hnd, operand, newGroup));
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
		out << "feature '" << fi->set << "':" << std::endl;
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
	switch (nodeType( m_stack.back()))
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

PostingIteratorInterface* Query::createExpressionPostingIterator( const Expression& expr)
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
				joinargs.push_back( m_storage->createTermPostingIterator(
							term.type, term.value));
				if (!joinargs.back().get()) throw strus::runtime_error(_TXT("error creating subexpression posting iterator"));

				m_nodePostingsMap[ *ni] = joinargs.back().get();
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ nodeIndex(*ni)]));
				if (!joinargs.back().get()) throw strus::runtime_error(_TXT("error creating subexpression posting iterator"));

				m_nodePostingsMap[ *ni] = joinargs.back().get();
				break;
		}
	}
	return expr.operation->createResultIterator( joinargs, expr.range, expr.cardinality);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& nodeadr)
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
			m_nodePostingsMap[ nodeadr] = rt;
			break;
		}
		case ExpressionNode:
			std::size_t nidx = nodeIndex( nodeadr);
			rt = createExpressionPostingIterator( m_expressions[ nidx]);
			if (!rt) break;
			m_nodePostingsMap[ nodeadr] = rt;
			break;
	}
	return rt;
}

PostingIteratorInterface* Query::nodePostings( const NodeAddress& nodeadr) const
{
	NodePostingsMap::const_iterator pi = m_nodePostingsMap.find( nodeadr);
	if (pi == m_nodePostingsMap.end())
	{
		throw strus::runtime_error( _TXT( "expression node postings not found"));
	}
	return pi->second;
}

void Query::collectSummarizationVariables(
			std::vector<SummarizationVariable>& variables,
			const NodeAddress& nodeadr)
{
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> vrange = m_variableAssignments.equal_range( nodeadr);

	Itr vi = vrange.first, ve = vrange.second;
	for (; vi != ve; ++vi)
	{
		variables.push_back( SummarizationVariable( vi->second, nodePostings( nodeadr)));
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
				collectSummarizationVariables( variables, *ni);
			}
			break;
		}
	}
}

std::vector<ResultDocument> Query::evaluate()
{
	try
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "evaluate query:" << std::endl;
		print( std::cout);
#endif
		// [1] Check initial conditions:
		if (m_nofRanks == 0)
		{
			return std::vector<ResultDocument>();
		}
		if (m_queryEval->weightingFunctions().empty())
		{
			m_errorhnd->report( _TXT( "cannot evaluate query, no weighting function defined"));
			return std::vector<ResultDocument>();
		}
		if (m_queryEval->selectionSets().empty())
		{
			m_errorhnd->report( _TXT( "cannot evaluate query, no selection features defined"));
			return std::vector<ResultDocument>();
		}
	
		// [3] Create the posting sets of the query features:
		std::vector<Reference<PostingIteratorInterface> > postings;
		{
			std::vector<Feature>::const_iterator
				fi = m_features.begin(), fe = m_features.end();
			for (; fi != fe; ++fi)
			{
				Reference<PostingIteratorInterface> postingsElem(
					createNodePostingIterator( fi->node));
				if (!postingsElem.get()) return std::vector<ResultDocument>();
				postings.push_back( postingsElem);
			}
		}
		// [4] Create the accumulator:
		DocsetPostingIterator evalset_itr;
		Accumulator accumulator(
			m_storage,
			m_metaDataReader.get(), m_metaDataRestrictions,
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
							nodePostings( fi->node), sidx, 
							nodeType( fi->node) == ExpressionNode);
					}
				}
			}
		}
		// [4.3] Add features for weighting:
		{
			std::vector<WeightingDef>::const_iterator
				wi = m_queryEval->weightingFunctions().begin(),
				we = m_queryEval->weightingFunctions().end();
			for (; wi != we; ++wi)
			{
				std::auto_ptr<WeightingFunctionContextInterface> execContext(
					wi->function()->createFunctionContext( m_storage, m_metaDataReader.get()));
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
							execContext->addWeightingFeature(
								si->parameterName(),
								nodePostings( fi->node),
								fi->weight);
#ifdef STRUS_LOWLEVEL_DEBUG
							std::cout << "add feature parameter " << si->parameterName() << "=" << fi->set << ' ' << fi->weight << std::endl;
#endif
						}
					}
				}
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cout << "add feature " << wi->functionName() << " weight " << wi->weight() << std::endl;
#endif
				accumulator.addFeature( wi->weight(), execContext.release());
			}
		}
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
							nodePostings( fi->node),
							nodeType( fi->node) == ExpressionNode, false);
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
							nodePostings( fi->node),
							nodeType( fi->node) == ExpressionNode, true);
					}
				}
			}
		}
		// [5] Do the ranking:
		std::vector<ResultDocument> rt;
		Ranker ranker( m_nofRanks + m_minRank);
	
		Index docno = 0;
		unsigned int state = 0;
		unsigned int prev_state = 0;
		float weight = 0.0;
	
		while (accumulator.nextRank( docno, state, weight))
		{
			ranker.insert( WeightedDocument( docno, weight));
			if (state > prev_state && ranker.nofRanks() >= m_nofRanks + m_minRank)
			{
				break;
			}
			prev_state = state;
		}
		std::vector<WeightedDocument>
			resultlist = ranker.result( m_minRank);
	
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
						m_storage, m_metaDataReader.get()));
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
							collectSummarizationVariables( variables, fi->node);
	
							closure->addSummarizationFeature(
								si->parameterName(), nodePostings(fi->node),
								variables, fi->weight);
						}
					}
				}
			}
		}
		// [7] Build the result:
		std::vector<WeightedDocument>::const_iterator
			ri=resultlist.begin(),re=resultlist.end();
	
		for (; ri != re; ++ri)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "result rank docno=" << ri->docno() << ", weight=" << ri->weight() << std::endl;
#endif
			std::vector<ResultDocument::Attribute> attr;
			std::vector<Reference<SummarizerFunctionContextInterface> >::iterator
				si = summarizers.begin(), se = summarizers.end();
	
			rt.push_back( ResultDocument( *ri));
			for (std::size_t sidx=0; si != se; ++si,++sidx)
			{
				std::vector<SummarizerFunctionContextInterface::SummaryElement>
					summary = (*si)->getSummary( ri->docno());
				std::vector<SummarizerFunctionContextInterface::SummaryElement>::const_iterator
					ci = summary.begin(), ce = summary.end();
				for (; ci != ce; ++ci)
				{
					rt.back().addAttribute(
							m_queryEval->summarizers()[sidx].resultAttribute(),
							ci->text(), ci->weight());
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error evaluating query: %s"), *m_errorhnd, std::vector<ResultDocument>());
}


