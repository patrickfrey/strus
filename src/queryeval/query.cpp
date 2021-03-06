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
#include "strus/storage/summaryElement.hpp"
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
Query::Query( const QueryEval* queryEval_, const StorageClientInterface* storage_, ErrorBufferInterface* errorhnd_)
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
		StructView feat;
		if (fi->weight != 1.0) feat( "weight", fi->weight);
		feat( "set", fi->set)( "struct", nodeView( fi->node));
		rt( feat);
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
	StructView rt;
	StructView var = variableView( adr);

	switch (nodeType( adr))
	{
		case NullNode:
			rt( "node", "NULL");
			break;
		case TermNode:
		{
			const Term& term = m_terms[ nodeIndex( adr)];
			rt("node", "term")("type", term.type)("value", term.value);
			break;
		}
		case ExpressionNode:
		{
			const Expression& expr = m_expressions[ nodeIndex( adr)];
			rt("node", "expression");
			rt("op", expr.operation->name());
			if (expr.range) rt("range", expr.range);
			if (expr.cardinality) rt("cardinality", expr.cardinality);
			std::vector<NodeAddress>::const_iterator
				ni = expr.subnodes.begin(),
				ne = expr.subnodes.end();
			StructView arg;
			for (; ni != ne; ++ni)
			{
				arg( nodeView( *ni));
			}
			rt("arg", arg);
			break;
		}
	}
	if (var.type() != StructView::Null)
	{
		rt( "var", var);
	}
	return rt;
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

PostingIteratorInterface* Query::createExpressionPostingIterator( const Expression& expr, NodeStorageDataMap& nodeStorageDataMap, bool usePosinfo) const
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
				if (usePosinfo)
				{
					joinargs.push_back( m_storage->createTermPostingIterator( term.type, term.value, term.length, getTermStatistics( term.type, term.value)));
				}
				else
				{
					joinargs.push_back( m_storage->createFrequencyPostingIterator( term.type, term.value, getTermStatistics( term.type, term.value)));
				}
				if (!joinargs.back().get()) throw std::runtime_error( _TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = joinargs.back().get();
				break;
			}
			case ExpressionNode:
				joinargs.push_back( createExpressionPostingIterator(
							m_expressions[ nodeIndex(*ni)], nodeStorageDataMap, usePosinfo));
				if (!joinargs.back().get()) throw std::runtime_error( _TXT("error creating subexpression posting iterator"));

				nodeStorageDataMap[ *ni] = joinargs.back().get();
				break;
		}
	}
	return expr.operation->createResultIterator( joinargs, expr.range, expr.cardinality);
}


PostingIteratorInterface* Query::createNodePostingIterator( const NodeAddress& nodeadr, NodeStorageDataMap& nodeStorageDataMap, bool usePosinfo) const
{
	PostingIteratorInterface* rt = 0;
	switch (nodeType( nodeadr))
	{
		case NullNode: break;
		case TermNode:
		{
			std::size_t nidx = nodeIndex( nodeadr);
			const Term& term = m_terms[ nidx];
			if (usePosinfo)
			{
				rt = m_storage->createTermPostingIterator( term.type, term.value, term.length, getTermStatistics( term.type, term.value));
			}
			else
			{
				rt = m_storage->createFrequencyPostingIterator( term.type, term.value, getTermStatistics( term.type, term.value));
			}
			if (!rt) break;
			nodeStorageDataMap[ nodeadr] = rt;
			break;
		}
		case ExpressionNode:
			std::size_t nidx = nodeIndex( nodeadr);
			rt = createExpressionPostingIterator( m_expressions[ nidx], nodeStorageDataMap, usePosinfo);
			if (!rt) break;
			nodeStorageDataMap[ nodeadr] = rt;
			break;
	}
	return rt;
}

PostingIteratorInterface* Query::nodeStorageData( const NodeAddress& nodeadr, const NodeStorageDataMap& nodeStorageDataMap) const
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
		PostingIteratorInterface* itr = nodeStorageData( nodeadr, nodeStorageDataMap);
		variables.push_back( SummarizationVariable( vi->second, itr));
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
				bool usePosinfo = m_queryEval->usePositionInformation( fi->set);
				Reference<PostingIteratorInterface> postingsElem(
					createNodePostingIterator( fi->node, nodeStorageDataMap, usePosinfo));
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
							nodeStorageData( fi->node, nodeStorageDataMap),
							sidx);
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
							PostingIteratorInterface* itr = nodeStorageData( fi->node, nodeStorageDataMap);
							execContext->addWeightingFeature( si->featureRole(), itr, fi->weight);
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
							nodeStorageData( fi->node, nodeStorageDataMap), false);
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
							nodeStorageData( fi->node, nodeStorageDataMap), true);
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
		Index docno = 0;
		unsigned int state = 0;
		unsigned int prev_state = 0;
	
		while (accumulator.nextRank( docno, state))
		{
			if (state > prev_state && (int)accumulator.ranker().nofRanks() >= maxNofRanks + minRank)
			{
				state = prev_state;
				break;
			}
			prev_state = state;
		}
		std::vector<WeightedDocument> resultlist = accumulator.ranker().result( minRank);

		// [6] Summarization:
		evaluationPhase = "summarization";
		std::vector<Reference<SummarizerFunctionContextInterface> > summarizers;
		if (!resultlist.empty())
		{
			// [6.1] Create the summarizers:
			std::vector<SummarizerDef>::const_iterator
				zi = m_queryEval->summarizers().begin(),
				ze = m_queryEval->summarizers().end();
			for (; zi != ze; ++zi)
			{
				// [5.1] Create the summarizer:
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

							PostingIteratorInterface* itr = nodeStorageData( fi->node, nodeStorageDataMap);
							closure->addSummarizationFeature(
								si->featureRole(), itr, variables, fi->weight);
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
		// [7.1] Build the ranklist and the map of populated summaries;
		typedef std::map<std::string,double> SummaryElementMap;
		std::map< std::string, SummaryElementMap> summaryMap;
		std::vector<WeightedDocument>::const_iterator ri=resultlist.begin(),re=resultlist.end();
		for (; ri != re; ++ri)
		{
			if (m_debugtrace) m_debugtrace->event( "result", "docno=%d weight=%f", ri->docno(), ri->weight());
			std::vector<SummaryElement> summaries;

			std::vector<Reference<SummarizerFunctionContextInterface> >::iterator
				si = summarizers.begin(), se = summarizers.end();
			for (int sidx=0 ;si != se; ++si,++sidx)
			{
				std::vector<SummaryElement> summary = (*si)->getSummary( *ri);
				std::vector<SummaryElement>::iterator li = summary.begin(), le = summary.end();
				for (; li != le; ++li)
				{
					li->setSummarizerPrefix( m_queryEval->summarizers()[ sidx].summaryId(), ':');
				}
				if (!ri->field().defined()
				&&  m_queryEval->summarizers()[ sidx].function()->doPopulate())
				{
					for (li = summary.begin(); li != le; ++li)
					{
						summaryMap[ li->name()][ li->value()] += li->weight();
					}
				}
				summaries.insert( summaries.end(), summary.begin(), summary.end());
			}
			if (m_debugtrace)
			{
				std::vector<SummaryElement>::const_iterator
					ai = summaries.begin(), ae = summaries.end();
				for (;ai != ae; ++ai)
				{
					if (ai->index() != -1)
					{
						if (ai->weight() != 1.0)
						{
							m_debugtrace->event( "summary", "name=%s value='%s' weight=%f index=%d", ai->name().c_str(), ai->value().c_str(), ai->weight(), ai->index());
						}
						else
						{
							m_debugtrace->event( "summary", "name=%s value='%s' index=%d", ai->name().c_str(), ai->value().c_str(), ai->index());
						}
					}
					else
					{
						if (ai->weight() != 1.0)
						{
							m_debugtrace->event( "summary", "name=%s value='%s' weight=%f", ai->name().c_str(), ai->value().c_str(), ai->weight());
						}
						else
						{
							m_debugtrace->event( "summary", "name=%s value='%s'", ai->name().c_str(), ai->value().c_str());
						}
					}
				}
			}
			ranks.push_back( ResultDocument( *ri, summaries));
		}
		// [7.2] Build the global summary from populated summary elements;
		std::vector<SummaryElement> summary;
		std::map<std::string, SummaryElementMap>::const_iterator
			si = summaryMap.begin(), se = summaryMap.end();
		for (; si != se; ++si)
		{
			double maxweight = 0.0;
			SummaryElementMap::const_iterator mi = si->second.begin(), me = si->second.end();
			for (; mi != me; ++mi)
			{
				if (maxweight < mi->second)
				{
					maxweight = mi->second;
				}
			}
			mi = si->second.begin();
			for (; mi != me; ++mi)
			{
				summary.push_back( SummaryElement( si->first, mi->first, mi->second / maxweight));
			}
		}
		if (m_errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("error evaluating query: %s"), m_errorhnd->fetchError());
		}
		if (m_debugtrace) m_debugtrace->close();/*ranking*/
		if (m_debugtrace) m_debugtrace->close();/*eval*/
		return QueryResult( state, accumulator.nofDocumentsRanked(), accumulator.nofDocumentsVisited(), ranks, summary);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error during %s when evaluating query: %s"), evaluationPhase, *m_errorhnd, QueryResult());
}


