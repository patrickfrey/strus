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
#include "queryProcessor.hpp"
#include "summarizer/summarizer_standard.hpp"
#include "iterator/iterator_standard.hpp"
#include "weighting/weighting_standard.hpp"
#include "strus/constants.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>
#include <iostream>

using namespace strus;

QueryProcessor::QueryProcessor( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_)
{
	PostingJoinOperatorInterface* op;
	if (0==(op=createPostingJoinWithin( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "within", op);
	if (0==(op=createPostingJoinStructWithin( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "within_struct", op);
	if (0==(op=createPostingJoinSequence( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "sequence", op);
	if (0==(op=createPostingJoinStructSequence( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "sequence_struct", op);
	if (0==(op=createPostingJoinDifference( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "diff", op);
	if (0==(op=createPostingJoinIntersect( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "intersect", op);
	if (0==(op=createPostingJoinUnion( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "union", op);
	if (0==(op=createPostingSucc( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "succ", op);
	if (0==(op=createPostingPred( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "pred", op);
	if (0==(op=createPostingJoinContains( m_errorhnd))) throw strus::runtime_error(_TXT("error creating posting join operator"));
	definePostingJoinOperator( "contains", op);

	WeightingFunctionInterface* func;
	if (0==(func=createWeightingFunctionBm25( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "bm25", func);
	if (0==(func=createWeightingFunctionBm25_dpfc( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "bm25_dpfc", func);
	if (0==(func=createWeightingFunctionTermFrequency( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "tf", func);
	if (0==(func=createWeightingFunctionConstant( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "td", func);
	if (0==(func=createWeightingFunctionMetadata( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "metadata", func);
	if (0==(func=createWeightingFunctionFormula( m_errorhnd))) throw strus::runtime_error(_TXT("error creating weighting function"));
	defineWeightingFunction( "formula", func);

	SummarizerFunctionInterface* sum;
	if (0==(sum=createSummarizerMetaData( m_errorhnd))) throw strus::runtime_error(_TXT("error creating summarizer"));
	defineSummarizerFunction( "metadata", sum);
	if (0==(sum=createSummarizerMatchPhrase( m_errorhnd))) throw strus::runtime_error(_TXT("error creating summarizer"));
	defineSummarizerFunction( "matchphrase", sum);
	if (0==(sum=createSummarizerListMatches( m_errorhnd))) throw strus::runtime_error(_TXT("error creating summarizer"));
	defineSummarizerFunction( "matchpos", sum);
	if (0==(sum=createSummarizerAttribute( m_errorhnd))) throw strus::runtime_error(_TXT("error creating summarizer"));
	defineSummarizerFunction( "attribute", sum);
	if (0==(sum=createSummarizerMatchVariables( m_errorhnd))) throw strus::runtime_error(_TXT("error creating summarizer"));
	defineSummarizerFunction( "matchvariables", sum);
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
		m_joiners[ utils::tolower( std::string(name))] = opref;
	}
	catch (std::bad_alloc&)
	{
		delete op;
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const PostingJoinOperatorInterface* QueryProcessor::getPostingJoinOperator(
		const std::string& name) const
{
	std::map<std::string,Reference<PostingJoinOperatorInterface> >::const_iterator 
		ji = m_joiners.find( utils::tolower( name));
	if (ji == m_joiners.end())
	{
		m_errorhnd->report( _TXT( "posting set join operator not defined: '%s'"), name.c_str());
		return 0;
	}
	return ji->second.get();
}

void QueryProcessor::defineWeightingFunction(
		const std::string& name,
		WeightingFunctionInterface* func)
{
	try
	{
		Reference<WeightingFunctionInterface> funcref( func);
		m_weighters[ utils::tolower( std::string(name))] = funcref;
	}
	catch (std::bad_alloc&)
	{
		delete func;
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const WeightingFunctionInterface* QueryProcessor::getWeightingFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<WeightingFunctionInterface> >::const_iterator 
		wi = m_weighters.find( utils::tolower( name));
	if (wi == m_weighters.end())
	{
		m_errorhnd->report( _TXT( "weighting function not defined: '%s'"), name.c_str());
		return 0;
	}
	return wi->second.get();
}

void QueryProcessor::defineSummarizerFunction(
		const std::string& name,
		SummarizerFunctionInterface* sumfunc)
{
	try
	{
		Reference<SummarizerFunctionInterface> funcref( sumfunc);
		m_summarizers[ utils::tolower( name)] = funcref;
	}
	catch (std::bad_alloc&)
	{
		delete sumfunc;
		m_errorhnd->report( _TXT("out of memory"));
	}
}

const SummarizerFunctionInterface* QueryProcessor::getSummarizerFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<SummarizerFunctionInterface> >::const_iterator 
		si = m_summarizers.find( utils::tolower( name));
	if (si == m_summarizers.end())
	{
		m_errorhnd->report( _TXT( "summarization function not defined: '%s'"), name.c_str());
		return 0;
	}
	return si->second.get();
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

template <class Map>
static const char* getDescription_( const Map& map, const std::string& name)
{
	typename Map::const_iterator mi = map.find( utils::tolower( name));
	if (mi != map.end())
	{
		return mi->second->getDescription();
	}
	return 0;
}

const char* QueryProcessor::getDescription( FunctionType type, const std::string& name) const
{
	try
	{
		switch (type)
		{
			case QueryProcessorInterface::PostingJoinOperator:
				return getDescription_( m_joiners, name);
			case QueryProcessorInterface::WeightingFunction:
				return getDescription_( m_weighters, name);
			case QueryProcessorInterface::SummarizerFunction:
				return getDescription_( m_summarizers, name);
		}
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory"));
	}
	return 0;
}

std::vector<std::string> QueryProcessor::getFunctionList( QueryProcessorInterface::FunctionType type) const
{
	try
	{
		switch (type)
		{
			case QueryProcessorInterface::PostingJoinOperator:
				return getKeys( m_joiners);
			case QueryProcessorInterface::WeightingFunction:
				return getKeys( m_weighters);
			case QueryProcessorInterface::SummarizerFunction:
				return getKeys( m_summarizers);
		}
	}
	catch (const std::bad_alloc&)
	{
		m_errorhnd->report( _TXT("out of memory"));
	}
	return std::vector<std::string>();
}

