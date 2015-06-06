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
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>
#include <iostream>

using namespace strus;

QueryProcessor::QueryProcessor()
{
	definePostingJoinOperator( "within", createPostingJoinWithin());
	definePostingJoinOperator( "within_struct", createPostingJoinStructWithin());
	definePostingJoinOperator( "sequence", createPostingJoinSequence());
	definePostingJoinOperator( "sequence_struct", createPostingJoinStructSequence());
	definePostingJoinOperator( "diff", createPostingJoinDifference());
	definePostingJoinOperator( "intersect", createPostingJoinIntersect());
	definePostingJoinOperator( "union", createPostingJoinUnion());
	definePostingJoinOperator( "succ", createPostingSucc());
	definePostingJoinOperator( "pred", createPostingPred());
	definePostingJoinOperator( "contains", createPostingJoinContains());
	
	defineWeightingFunction( "bm25", createWeightingFunctionBm25());
	defineWeightingFunction( "bm25_dpfc", createWeightingFunctionBm25_dpfc());
	defineWeightingFunction( "tf", createWeightingFunctionTermFrequency());
	defineWeightingFunction( "td", createWeightingFunctionConstant());
	defineWeightingFunction( "metadata", createWeightingFunctionMetadata());

	defineSummarizerFunction( "metadata", createSummarizerMetaData());
	defineSummarizerFunction( "matchphrase", createSummarizerMatchPhrase());
	defineSummarizerFunction( "matchpos", createSummarizerListMatches());
	defineSummarizerFunction( "attribute", createSummarizerAttribute());
	defineSummarizerFunction( "matchvariables", createSummarizerMatchVariables());
}

void QueryProcessor::definePostingJoinOperator(
		const std::string& name,
		PostingJoinOperatorInterface* op)
{
	Reference<PostingJoinOperatorInterface> opref( op);
	m_joiners[ utils::tolower( std::string(name))] = opref;
}

const PostingJoinOperatorInterface* QueryProcessor::getPostingJoinOperator(
		const std::string& name) const
{
	std::map<std::string,Reference<PostingJoinOperatorInterface> >::const_iterator 
		ji = m_joiners.find( utils::tolower( name));
	if (ji == m_joiners.end())
	{
		throw strus::runtime_error( _TXT( "posting set join operator not defined: '%s'"), name.c_str());
	}
	return ji->second.get();
}

void QueryProcessor::defineWeightingFunction(
		const std::string& name,
		WeightingFunctionInterface* func)
{
	Reference<WeightingFunctionInterface> funcref( func);
	m_weighters[ utils::tolower( std::string(name))] = funcref;
}

const WeightingFunctionInterface* QueryProcessor::getWeightingFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<WeightingFunctionInterface> >::const_iterator 
		wi = m_weighters.find( utils::tolower( std::string(name)));
	if (wi == m_weighters.end())
	{
		throw strus::runtime_error( _TXT( "weighting function not defined: '%s'"), name.c_str());
	}
	return wi->second.get();
}

void QueryProcessor::defineSummarizerFunction(
		const std::string& name,
		SummarizerFunctionInterface* sumfunc)
{
	Reference<SummarizerFunctionInterface> funcref( sumfunc);
	m_summarizers[ utils::tolower( std::string(name))] = funcref;
}

const SummarizerFunctionInterface* QueryProcessor::getSummarizerFunction(
		const std::string& name) const
{
	std::map<std::string,Reference<SummarizerFunctionInterface> >::const_iterator 
		si = m_summarizers.find( utils::tolower( std::string(name)));
	if (si == m_summarizers.end())
	{
		throw strus::runtime_error( _TXT( "summarization function not defined: '%s'"), name.c_str());
	}
	return si->second.get();
}



