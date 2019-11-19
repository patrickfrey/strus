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
#include "strus/base/snprintf.h"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>

#define STRUS_DBGTRACE_COMPONENT_NAME "query"

using namespace strus;

///\brief Constructor
Query::Query( const QueryEval* queryEval_, const StorageClientInterface* storage_, bool usePosinfo_, ErrorBufferInterface* errorhnd_)
	:m_queryEval(queryEval_)
	,m_storage(storage_)
	,m_metaDataReader(storage_->createMetaDataReader())
	,m_metaDataRestriction()
	,m_weightingFormula()
	,m_terms()
	,m_expressions()
	,m_features()
	,m_stack()
	,m_variableAssignments()
	,m_usernames()
	,m_evalset_docnolist()
	,m_evalset_defined(false)
	,m_termstatsmap()
	,m_globstats()
	,m_debugMode(false)
	,m_usePosinfo(usePosinfo_)
	,m_errorhnd(errorhnd_)
	,m_debugtrace(0)
{
	DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
	if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);

	if (!m_metaDataReader.get()) throw std::runtime_error( _TXT("error creating meta data reader"));
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
		pushTerm( ti->type, ti->value, 1);
		defineFeature( ti->set, 1.0);
	}
}

Query::~Query()
{
	if (m_debugtrace) delete m_debugtrace;
}

bool Query::TermKey::operator<( const TermKey& o) const
{
	int cmpres;
	if (type.size() != o.type.size()) return type.size() < o.type.size();
	if ((cmpres = std::strcmp( type.c_str(), o.type.c_str())) < 0) return cmpres;
	if (value.size() != o.value.size()) return value.size() < o.value.size();
	if ((cmpres = std::strcmp( value.c_str(), o.value.c_str())) < 0) return cmpres;
	return 0;
}

std::string Query::printStack() const
{
	std::ostringstream out;
	std::vector<NodeAddress>::const_iterator si = m_stack.begin(), se = m_stack.end();
	std::size_t sidx = m_stack.size();
	out << "Stack" << std::endl;
	for (--sidx; si != se; ++si,--sidx)
	{
		out << "STK [" << sidx << "] " << nodeView( *si).tostring() << std::endl;
	}
	return out.str();
}

void Query::pushTerm( const std::string& type_, const std::string& value_, const Index& length_)
{
	if (m_debugtrace) m_debugtrace->event( "term", "type='%s' value='%s' len=%d", type_.c_str(), value_.c_str(), length_);
	try
	{
		m_terms.push_back( Term( type_, value_, length_));
		m_stack.push_back( nodeAddress( TermNode, m_terms.size()-1));
	}
	CATCH_ERROR_MAP( _TXT("error pushing term to query: %s"), *m_errorhnd);
}

void Query::pushExpression( const PostingJoinOperatorInterface* operation, unsigned int argc, int range_, unsigned int cardinality_)
{
	if (m_debugtrace) m_debugtrace->event( "expression", "argc=%d range=%d cardinality=%d", argc, range_, cardinality_);
	try
	{
		if (argc > m_stack.size())
		{
			m_errorhnd->report( ErrorCodeValueOutOfRange, _TXT( "illegal expression definition: size of expression bigger than stack size"));
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
	if (m_debugtrace) m_debugtrace->event( "variable", "name='%s'", name_.c_str());
	try
	{
		if (m_stack.empty())
		{
			m_errorhnd->report( ErrorCodeInvalidOperation, _TXT( "cannot attach variable (query stack empty)"));
		}
		m_variableAssignments.insert( std::pair<NodeAddress,std::string>( m_stack.back(), name_));
	}
	CATCH_ERROR_MAP( _TXT("error attaching variables to query: %s"), *m_errorhnd);
}

void Query::defineFeature( const std::string& set_, double weight_)
{
	if (m_debugtrace)
	{
		std::string stkstr = printStack();
		m_debugtrace->event( "stack", "%s", stkstr.c_str());
		m_debugtrace->event( "feature", "set=%s weight=%f", set_.c_str(), weight_);
	}
	try
	{
		if (m_stack.empty()) throw std::runtime_error( _TXT("no term or expression defined"));
		m_features.push_back( Feature( string_conv::tolower(set_), m_stack.back(), weight_));
		m_stack.pop_back();
	}
	CATCH_ERROR_MAP( _TXT("error define feature of query: %s"), *m_errorhnd);
}

void Query::addMetaDataRestrictionCondition(
		const MetaDataRestrictionInterface::CompareOperator& opr, const std::string&  name,
		const NumericVariant& operand, bool newGroup)
{
	try
	{
		if (m_debugtrace) m_debugtrace->event( "restriction", "op='%s' name='%s' operand='%s' newgroup=%s", MetaDataRestrictionInterface::compareOperatorStr(opr), name.c_str(), operand.tostring().c_str(), newGroup?"true":"false");
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
		if (m_debugtrace)
		{
			std::string docnoliststr;
			std::vector<strus::Index>::const_iterator
				di=docnolist_.begin(), de=docnolist_.end();
			for (; di != de; ++di)
			{
				char buf[ 64];
				std::snprintf( buf, sizeof( buf), docnoliststr.empty() ? "%d":" %d", (int)*di);
				docnoliststr.append( buf);
			}
			m_debugtrace->event( "evalset", "%s", docnoliststr.c_str());
		}
		m_evalset_docnolist.insert( m_evalset_docnolist.end(), docnolist_.begin(), docnolist_.end());
		std::sort( m_evalset_docnolist.begin(), m_evalset_docnolist.end());
		m_evalset_defined = true;
	}
	CATCH_ERROR_MAP( _TXT("error define document evaluation set of query: %s"), *m_errorhnd);
}

StructView Query::view() const
{
	StructView rt;
	rt( "eval", m_queryEval->view());
	rt( "feature", featuresView());
	rt( "debug", m_debugMode ? "true":"false");
	if (!m_evalset_docnolist.empty()) rt( "docs", m_evalset_docnolist);
	if (!m_usernames.empty()) rt( "user", m_usernames);
	return rt;
}

StructView Query::featuresView() const
{
	StructView rt;
	std::vector<Feature>::const_iterator fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		rt( StructView()( "weight", fi->weight)( "set", fi->set)( "struct", nodeView( fi->node)));
	}
	return rt;
}

StructView Query::variableView( NodeAddress adr) const
{
	StructView rt;
	typedef std::multimap<NodeAddress,std::string>::const_iterator Itr;
	std::pair<Itr,Itr> variables = m_variableAssignments.equal_range( adr);

	if (variables.first != variables.second)
	{
		Itr vi = variables.first, ve = variables.second;
		for (std::size_t vidx=0; vi != ve; ++vi,++vidx)
		{
			rt( vi->second);
		}
	}
	return rt;
}

StructView Query::nodeView( NodeAddress adr) const
{
	switch (nodeType( adr))
	{
		case NullNode:
			return StructView()( "node", "NULL")( "var", variableView( adr));
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( adr)];
			return StructView()
				("node", "term")
				("type", term.type)
				("value", term.value)
				("var", variableView( adr));
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			StructView rt;
			rt("node", "expression");
			rt("op", expr.operation->name());
			rt("range", expr.range);
			rt("cardinality", expr.cardinality);
			rt("var", variableView( adr));
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(),
				ne = expr.subnodes.end();
			StructView arg;
			for (; ni != ne; ++ni)
			{
				arg( nodeView( *ni));
			}
			rt("arg", arg);
			return rt;
		}
	}
	return StructView();
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

void Query::addAccess( const std::string& username_)
{
	try
	{
		m_usernames.push_back( username_);
	}
	CATCH_ERROR_MAP( _TXT("error adding user to query: %s"), *m_errorhnd);
}

PostingIteratorInterface* Query::createExpressionPostingIterator( const Expression& expr, NodeStorageDataMap& nodeStorageDataMap) const
{
	if (expr.subnodes.size() > MaxNofJoinopArguments)
	{
		throw strus::runtime_error( "%s",  _TXT( "number of arguments of feature join expression in query out of range"));
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
				if (m_usePosinfo)
				{
					joinargs.push_back( m_storage->createTermPostingIterator( term.type, term.value, term.length));
				}
				else
				{
					joinargs.push_back( m_storage->createFrequencyPostingIterator( term.type, term.value));
				}
				if (!joinargs.back().get()) throw std::runtime_error( _TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = NodeStorageData( joinargs.back().get(), getTermStatistics( term.type, term.value));
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ nodeIndex(*ni)], nodeStorageDataMap));
				if (!joinargs.back().get()) throw std::runtime_error( _TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = NodeStorageData( joinargs.back().get());
				break;
		}
	}
	return expr.operation->createResultIterator( joinargs, expr.range, expr.cardinality);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& nodeadr, NodeStorageDataMap& nodeStorageDataMap) const
{
	PostingIteratorInterface* rt = 0;
	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Term& term = m_terms[ nidx];
			if (m_usePosinfo)
			{
				rt = m_storage->createTermPostingIterator( term.type, term.value, term.length);
			}
			else
			{
				rt = m_storage->createFrequencyPostingIterator( term.type, term.value);
			}
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
		throw strus::runtime_error( "%s",  _TXT( "expression node postings not found"));
	}
	return pi->second;
}

void Query::collectSummarizationVariables(
			std::vector<SummarizationVariable>& variables,
			const NodeAddress& nodeadr,
			const NodeStorageDataMap& nodeStorageDataMap) const
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
	m_termstatsmap[ TermKey( type_, value_)] = stats_;
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
	TermStatisticsMap::const_iterator si = m_termstatsmap.find( TermKey( type_, value_));
	if (si == m_termstatsmap.end()) return undef;
	return si->second;
}

void Query::setWeightingVariableValue(
		const std::string& name, double value)
{
	std::vector<QueryEval::VariableAssignment>
		assignments = m_queryEval->weightingVariableAssignmentList( name);
	std::vector<QueryEval::VariableAssignment>::const_iterator
		ai = assignments.begin(), ae = assignments.end();
	for (; ai != ae; ++ai)
	{
		switch (ai->target)
		{
			case QueryEval::VariableAssignment::WeightingFunction:
				m_weightingvars.push_back( WeightingVariableValueAssignment( name, ai->index, value));
				break;
			case QueryEval::VariableAssignment::SummarizerFunction:
				m_summaryweightvars.push_back( WeightingVariableValueAssignment( name, ai->index, value));
				break;
			case QueryEval::VariableAssignment::FormulaFunction:
				if (!m_weightingFormula.get())
				{
					m_errorhnd->report( ErrorCodeOperationOrder, _TXT("try to defined weighting variable without weighting formula defined"));
				}
				m_weightingFormula->setVariableValue( name, value);
				break;
		}
	}
}

void Query::setDebugMode( bool debug)
{
	m_debugMode = debug;
}

QueryResult Query::evaluate( int minRank, int maxNofRanks) const
{
	const char* evaluationPhase = "query feature postings initialization";
	try
	{
		if (m_debugtrace)
		{
			m_debugtrace->open( "eval");
			std::string str = view().tostring();
			m_debugtrace->event( "query", "%s", str.c_str());
		}
		// [1] Check initial conditions:
		if (maxNofRanks == 0)
		{
			if (m_debugtrace) m_debugtrace->close();
			return QueryResult();
		}
		if (m_queryEval->weightingFunctions().empty())
		{
			m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "cannot evaluate query, no weighting function defined"));
			if (m_debugtrace) m_debugtrace->close();
			return QueryResult();
		}
		if (m_queryEval->selectionSets().empty())
		{
			m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "cannot evaluate query, no selection features defined"));
			if (m_debugtrace) m_debugtrace->close();
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
				if (!postingsElem.get())
				{
					if (m_debugtrace) m_debugtrace->close();
					return QueryResult();
				}
				postings.push_back( postingsElem);
			}
		}
		// [4] Create the accumulator:
		DocsetPostingIterator evalset_itr;
		Accumulator accumulator(
			m_storage,
			m_metaDataReader.get(), m_metaDataRestriction.get(), m_weightingFormula.get(),
			minRank + maxNofRanks, m_storage->maxDocumentNumber());

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
		// [4.3.1] Add features for weighting:
		{
			std::vector<WeightingDef>::const_iterator
				wi = m_queryEval->weightingFunctions().begin(),
				we = m_queryEval->weightingFunctions().end();
			for (; wi != we; ++wi)
			{
				if (m_debugtrace)
				{
					m_debugtrace->open( "function");
					m_debugtrace->event( "name", "%s", wi->function()->name());
				}
				strus::local_ptr<WeightingFunctionContextInterface> execContext(
					wi->function()->createFunctionContext( m_storage, m_globstats));
				if (!execContext.get()) throw std::runtime_error( _TXT("error creating weighting function context"));

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
								si->featureRole(), nd.itr, fi->weight, nd.stats);
							if (m_debugtrace) m_debugtrace->event( "parameter", "%s= feature %s weight=%f", si->featureRole().c_str(), fi->set.c_str(), fi->weight);
						}
					}
				}
				accumulator.addWeightingElement( execContext.release());
				if (m_debugtrace) m_debugtrace->close();
			}
		}
		// [4.3.1] Define feature weighting variable values:
		std::vector<WeightingVariableValueAssignment>::const_iterator
			vi = m_weightingvars.begin(), ve = m_weightingvars.end();
		for (; vi != ve; ++vi)
		{
			if (m_debugtrace) m_debugtrace->event( "variable", "index=%d name=%s value=%f", (int)vi->index, vi->varname.c_str(), vi->value);
			accumulator.defineWeightingVariableValue( vi->index, vi->varname, vi->value);
		}

		evaluationPhase = "restrictions initialization";
		// [4.4] Define the user ACL restrictions:
		std::vector<std::string>::const_iterator ui = m_usernames.begin(), ue = m_usernames.end();
		for (; ui != ue; ++ui)
		{
			if (m_debugtrace) m_debugtrace->event( "user-restriction", "name=%s", ui->c_str());
			Reference<InvAclIteratorInterface> invAcl( m_storage->createInvAclIterator( *ui));
			if (invAcl.get())
			{
				accumulator.addAlternativeAclRestriction( invAcl);
			}
			else if (m_errorhnd->hasError())
			{
				throw std::runtime_error( _TXT( "storage built without ACL resrictions, cannot handle username passed with query"));
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
						if (m_debugtrace) m_debugtrace->event( "feature-restriction", "name=%s", xi->c_str());
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
						if (m_debugtrace) m_debugtrace->event( "feature-exclusion", "name=%s", xi->c_str());
						accumulator.addFeatureRestriction(
							nodeStorageData( fi->node, nodeStorageDataMap).itr, true);
					}
				}
			}
		}
		if (m_debugtrace)
		{
			m_debugtrace->open( "ranking");
		}
		evaluationPhase = "document ranking";
		// [5] Do the ranking:
		std::vector<ResultDocument> ranks;
		Ranker ranker( maxNofRanks + minRank);
		Index docno = 0;
		unsigned int state = 0;
		unsigned int prev_state = 0;
		double weight = 0.0;
	
		while (accumulator.nextRank( docno, state, weight))
		{
			ranker.insert( WeightedDocument( docno, weight));
			if (state > prev_state && (int)ranker.nofRanks() >= maxNofRanks + minRank)
			{
				state = prev_state;
				break;
			}
			prev_state = state;
		}
		std::vector<WeightedDocument> resultlist = ranker.result( minRank);
	
		// [6] Summarization:
		evaluationPhase = "summarization";
		std::vector<Reference<SummarizerFunctionContextInterface> > summarizers;
		std::vector<std::string> summarizerPrefixList;
		if (!resultlist.empty())
		{
			// [6.1] Create the summarizers:
			std::vector<SummarizerDef>::const_iterator
				zi = m_queryEval->summarizers().begin(),
				ze = m_queryEval->summarizers().end();
			for (; zi != ze; ++zi)
			{
				// [5.1] Create the summarizer:
				summarizerPrefixList.push_back( zi->summaryId().empty() ? std::string() : (zi->summaryId() + ":"));
				summarizers.push_back(
					zi->function()->createFunctionContext( m_storage, m_globstats));
				SummarizerFunctionContextInterface* closure = summarizers.back().get();
				if (!closure) throw std::runtime_error( _TXT("error creating summarizer context"));

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
								si->featureRole(), nd.itr,
								variables, fi->weight, nd.stats);
						}
					}
				}
			}
			// [6.2] Define feature summarizer weighting variable values:
			vi = m_summaryweightvars.begin(), ve = m_summaryweightvars.end();
			for (; vi != ve; ++vi)
			{
				summarizers[ vi->index]->setVariableValue( vi->varname, vi->value);
			}
		}

		evaluationPhase = "building of the result";
		// [7] Build the result:
		std::vector<WeightedDocument>::const_iterator ri=resultlist.begin(),re=resultlist.end();
		for (; ri != re; ++ri)
		{
			if (m_debugtrace) m_debugtrace->event( "result", "docno=%d weight=%f", ri->docno(), ri->weight());
			std::vector<SummaryElement> summaries;

			std::vector<std::string>::const_iterator
				prefix_si = summarizerPrefixList.begin(),
				prefix_se = summarizerPrefixList.end();
			std::vector<Reference<SummarizerFunctionContextInterface> >::iterator
				si = summarizers.begin(), se = summarizers.end();
			for (;si != se; ++si,++prefix_si)
			{
				std::vector<SummaryElement> summary = (*si)->getSummary( ri->docno());
				std::vector<SummaryElement>::iterator li = summary.begin(), le = summary.end();
				for (; li != le; ++li)
				{
					li->setSummarizerPrefix( *prefix_si);
				}
				summaries.insert( summaries.end(), summary.begin(), summary.end());
			}
			if (m_debugMode)
			{
				// Collect debug info of weighting and summarizer functions:
				std::vector<SummarizerDef>::const_iterator
					zi = m_queryEval->summarizers().begin(),
					ze = m_queryEval->summarizers().end();
				si = summarizers.begin();
				for (;si != se && zi != ze; ++si,++zi)
				{
					if (!zi->debugAttributeName().empty())
					{
						std::string debuginfo = (*si)->debugCall( ri->docno());
						summaries.push_back( SummaryElement( std::string("debug:") + zi->debugAttributeName(), debuginfo));
					}
				}
				std::vector<WeightingDef>::const_iterator
					wi = m_queryEval->weightingFunctions().begin(),
					we = m_queryEval->weightingFunctions().end();
				std::size_t widx = 0;
				for (; wi != we; ++wi,++widx)
				{
					if (!wi->debugAttributeName().empty())
					{
						std::string debuginfo = accumulator.getWeightingDebugInfo( widx, ri->docno());
						summaries.push_back( SummaryElement( std::string("debug:") + wi->debugAttributeName(), debuginfo));
					}
				}
			}
			if (m_debugtrace)
			{
				std::vector<SummaryElement>::const_iterator
					ai = summaries.begin(), ae = summaries.end();
				for (;ai != ae; ++ai)
				{
					if (m_debugtrace) m_debugtrace->event( "summary", "name=%s value='%s' weight=%f index=%d", ai->name().c_str(), ai->value().c_str(), ai->weight(), ai->index());
				}
			}
			ranks.push_back( ResultDocument( *ri, summaries));
		}
		if (m_errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("error evaluating query: %s"), m_errorhnd->fetchError());
		}
		if (m_debugtrace) m_debugtrace->close();/*ranking*/
		if (m_debugtrace) m_debugtrace->close();/*eval*/
		return QueryResult( state, accumulator.nofDocumentsRanked(), accumulator.nofDocumentsVisited(), ranks);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error during %s when evaluating query: %s"), evaluationPhase, *m_errorhnd, QueryResult());
}


