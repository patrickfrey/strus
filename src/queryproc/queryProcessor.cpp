/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "queryProcessor.hpp"
#include "summarizer/summarizer_standard.hpp"
#include "iterator/iterator_standard.hpp"
#include "weighting/weighting_standard.hpp"
#include "strus/lib/scalarfunc.hpp"
#include "strus/constants.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/string_conv.hpp"
#include "weightingFunctionSummarizer.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>
#include <iostream>

using namespace strus;

#define DEFAULT_SCALAR_FUNCTION_PARSER "default"

QueryProcessor::QueryProcessor( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_)
{
	PostingJoinOperatorInterface* op;
	if (0==(op=createPostingJoinInRange( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "inrange", op);
	if (0==(op=createPostingJoinStructInRange( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "inrange_struct", op);
	if (0==(op=createPostingJoinWithin( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "within", op);
	if (0==(op=createPostingJoinStructWithin( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "within_struct", op);
	if (0==(op=createPostingJoinSequence( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "sequence", op);
	if (0==(op=createPostingJoinStructSequence( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "sequence_struct", op);
	if (0==(op=createPostingJoinChain( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "chain", op);
	if (0==(op=createPostingJoinStructChain( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "chain_struct", op);
	if (0==(op=createPostingJoinSequenceImm( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "sequence_imm", op);
	if (0==(op=createPostingJoinDifference( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "diff", op);
	if (0==(op=createPostingJoinIntersect( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "intersect", op);
	if (0==(op=createPostingJoinUnion( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "union", op);
	if (0==(op=createPostingSucc( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "succ", op);
	if (0==(op=createPostingPred( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "pred", op);
	if (0==(op=createPostingJoinContains( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating posting join operator"));
	definePostingJoinOperator( "contains", op);

	WeightingFunctionInterface* func;
	SummarizerFunctionInterface* sum;

	if (0==(func=createWeightingFunctionBm25( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "bm25", func);
	if (0==(func=createWeightingFunctionBm25pff( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "bm25pff", func);
	if (0==(func=createWeightingFunctionTermFrequency( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "tf", func);
	if (0==(func=createWeightingFunctionConstant( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "constant", func);
	if (0==(func=createWeightingFunctionMetadata( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "metadata", func);

	if (0==(func=createWeightingFunctionSmart( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "smart", func);
	if (0==(sum=createSummarizerFromWeightingFunction( "smart", m_errorhnd, func))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "smart", sum);

	if (0==(func=createWeightingFunctionScalar( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating weighting function"));
	defineWeightingFunction( "scalar", func);
	if (0==(sum=createSummarizerFromWeightingFunction( "scalar", m_errorhnd, func))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "scalar", sum);

	if (0==(sum=createSummarizerMetaData( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "metadata", sum);
	if (0==(sum=createSummarizerAttribute( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "attribute", sum);
	if (0==(sum=createSummarizerForwardIndex( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "forwardindex", sum);
	if (0==(sum=createSummarizerMatchPhrase( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "matchphrase", sum);
	if (0==(sum=createSummarizerListMatches( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "matchpos", sum);
	if (0==(sum=createSummarizerMatchVariables( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "matchvar", sum);
	if (0==(sum=createSummarizerAccumulateVariable( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "accuvar", sum);
	if (0==(sum=createSummarizerAccumulateNear( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating summarizer"));
	defineSummarizerFunction( "accunear", sum);

	ScalarFunctionParserInterface* sfp;
	if (0==(sfp=createScalarFunctionParser_default( m_errorhnd))) throw strus::runtime_error( "%s", _TXT("error creating scalar function parser"));
	defineScalarFunctionParser( DEFAULT_SCALAR_FUNCTION_PARSER, sfp);
}

QueryProcessor::~QueryProcessor()
{}

void QueryProcessor::definePostingJoinOperator(
		const std::string& name,
		PostingJoinOperatorInterface* op)
{
	try
	{
		Reference<PostingJoinOperatorInterface> opref( op);
		m_joiners[ string_conv::tolower( std::string(name))] = opref;
	}
	catch (std::bad_alloc&)
	{
		delete op;
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
	}
}

const PostingJoinOperatorInterface* QueryProcessor::getPostingJoinOperator(
		const std::string& name) const
{
	try
	{
		std::map<std::string,Reference<PostingJoinOperatorInterface> >::const_iterator 
			ji = m_joiners.find( string_conv::tolower( name));
		if (ji == m_joiners.end())
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT( "posting set join operator not defined: '%s'"), name.c_str());
			return 0;
		}
		return ji->second.get();
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
		return 0;
	}
}

void QueryProcessor::defineWeightingFunction(
		const std::string& name,
		WeightingFunctionInterface* func)
{
	try
	{
		Reference<WeightingFunctionInterface> funcref( func);
		m_weighters[ string_conv::tolower( std::string(name))] = funcref;
	}
	catch (std::bad_alloc&)
	{
		delete func;
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
	}
}

const WeightingFunctionInterface* QueryProcessor::getWeightingFunction(
		const std::string& name) const
{
	try
	{
		std::map<std::string,Reference<WeightingFunctionInterface> >::const_iterator 
			wi = m_weighters.find( string_conv::tolower( name));
		if (wi == m_weighters.end())
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT( "weighting function not defined: '%s'"), name.c_str());
			return 0;
		}
		return wi->second.get();
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
		return 0;
	}
}

void QueryProcessor::defineSummarizerFunction(
		const std::string& name,
		SummarizerFunctionInterface* sumfunc)
{
	try
	{
		Reference<SummarizerFunctionInterface> funcref( sumfunc);
		m_summarizers[ string_conv::tolower( name)] = funcref;
	}
	catch (std::bad_alloc&)
	{
		delete sumfunc;
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
	}
}

const SummarizerFunctionInterface* QueryProcessor::getSummarizerFunction(
		const std::string& name) const
{
	try
	{
		std::map<std::string,Reference<SummarizerFunctionInterface> >::const_iterator 
			si = m_summarizers.find( string_conv::tolower( name));
		if (si == m_summarizers.end())
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT( "summarization function not defined: '%s'"), name.c_str());
			return 0;
		}
		return si->second.get();
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
		return 0;
	}
}

template <class Map>
static std::vector<std::string> getKeys( const Map& map)
{
	std::vector<std::string> rt;
	typename Map::const_iterator mi = map.begin(), me = map.end();
	for (; mi != me; ++mi)
	{
		rt.push_back( mi->first);
	}
	return rt;
}

std::vector<std::string> QueryProcessor::getFunctionList( const FunctionType& type) const
{
	try
	{
		switch (type)
		{
			case PostingJoinOperator:
				return getKeys( m_joiners);
			case WeightingFunction:
				return getKeys( m_weighters);
			case SummarizerFunction:
				return getKeys( m_summarizers);
			case ScalarFunctionParser:
				return getKeys( m_funcparsers);
		}
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
	}
	return std::vector<std::string>();
}

void QueryProcessor::defineScalarFunctionParser(
		const std::string& name,
		ScalarFunctionParserInterface* parser)
{
	try
	{
		Reference<ScalarFunctionParserInterface> funcref( parser);
		m_funcparsers[ string_conv::tolower( name)] = funcref;
	}
	catch (std::bad_alloc&)
	{
		delete parser;
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
	}
	
}

const ScalarFunctionParserInterface*
	QueryProcessor::getScalarFunctionParser(
		const std::string& name) const
{
	try
	{
		std::map<std::string,Reference<ScalarFunctionParserInterface> >::const_iterator 
			si = m_funcparsers.find( name.empty() ? std::string(DEFAULT_SCALAR_FUNCTION_PARSER) : string_conv::tolower( name));
		if (si == m_funcparsers.end())
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT( "scalar function parser not defined: '%s'"), name.c_str());
			return 0;
		}
		return si->second.get();
	}
	catch (std::bad_alloc&)
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseOutOfMem), _TXT("out of memory"));
		return 0;
	}
}

