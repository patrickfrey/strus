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
#include "strus/constants.hpp"
#include "strus/storageInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "iterator/postingIteratorContains.hpp"
#include "iterator/postingIteratorPred.hpp"
#include "iterator/postingIteratorSucc.hpp"
#include "iterator/postingIteratorIntersect.hpp"
#include "iterator/postingIteratorUnion.hpp"
#include "iterator/postingIteratorDifference.hpp"
#include "iterator/postingIteratorStructWithin.hpp"
#include "iterator/postingIteratorStructSequence.hpp"
#include "weighting/weightingConstant.hpp"
#include "weighting/weightingFrequency.hpp"
#include "weighting/weightingBM25.hpp"
#include "summarizer/summarizerMetaData.hpp"
#include "summarizer/summarizerAttribute.hpp"
#include "summarizer/summarizerMatchPhrase.hpp"
#include "summarizer/summarizerListMatches.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <limits>
#include <iostream>
#include <boost/algorithm/string.hpp>

using namespace strus;

// Predefined functions:
static const WeightingFunctionBM25 weightingBM25;
static const WeightingFunctionTermFrequency weightingTF;
static const WeightingFunctionConstant weightingTD;

static const SummarizerFunctionMetaData summarizerMetaData;
static const SummarizerFunctionMatchPhrase summarizerMatchPhrase;
static const SummarizerFunctionListMatches summarizerListMatches;
static const SummarizerFunctionAttribute summarizerAttribute;

static const PostingJoinWithin joinWithin;
static const PostingJoinStructWithin joinWithinStruct;
static const PostingJoinSequence joinSequence;
static const PostingJoinStructSequence joinSequenceStruct;
static const PostingJoinDifference joinDiff;
static const PostingJoinIntersect joinIntersect;
static const PostingJoinContains joinContains;
static const PostingJoinUnion joinUnion;
static const PostingJoinSucc joinSucc;
static const PostingJoinPred joinPred;


QueryProcessor::QueryProcessor( StorageInterface* storage_)
	:m_storage(storage_)
{
	definePostingJoinOperator( "within", &joinWithin);
	definePostingJoinOperator( "within_struct", &joinWithinStruct);
	definePostingJoinOperator( "sequence", &joinSequence);
	definePostingJoinOperator( "sequence_struct", &joinSequenceStruct);
	definePostingJoinOperator( "diff", &joinDiff);
	definePostingJoinOperator( "intersect", &joinIntersect);
	definePostingJoinOperator( "union", &joinUnion);
	definePostingJoinOperator( "succ", &joinSucc);
	definePostingJoinOperator( "pred", &joinPred);
	definePostingJoinOperator( "contains", &joinContains);

	defineWeightingFunction( "BM25", &weightingBM25);
	defineWeightingFunction( "TF", &weightingTF);
	defineWeightingFunction( "TD", &weightingTD);

	defineSummarizerFunction( "metadata", &summarizerMetaData);
	defineSummarizerFunction( "matchphrase", &summarizerMatchPhrase);
	defineSummarizerFunction( "matchpos", &summarizerListMatches);
	defineSummarizerFunction( "attribute", &summarizerAttribute);
}

PostingIteratorInterface*
	QueryProcessor::createTermPostingIterator( 
			const std::string& type,
			const std::string& value) const
{
	PostingIteratorInterface* rt = m_storage->createTermPostingIterator( type, value);
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "create term " << value << ":" << type << " (" << rt->featureid() << ")" << std::endl;
#endif
	return rt;
}


void QueryProcessor::definePostingJoinOperator(
		const char* name,
		const PostingJoinOperatorInterface* op)
{
	m_joiners[ boost::algorithm::to_lower_copy( std::string(name))] = op;
}

const PostingJoinOperatorInterface* QueryProcessor::getPostingJoinOperator(
		const std::string& name) const
{
	std::map<std::string,const PostingJoinOperatorInterface*>::const_iterator 
		ji = m_joiners.find( boost::algorithm::to_lower_copy( name));
	if (ji == m_joiners.end())
	{
		throw std::runtime_error( std::string( "posting set join operator not defined: '") + name + "'");
	}
	return ji->second;
}

void QueryProcessor::defineWeightingFunction(
		const char* name,
		const WeightingFunctionInterface* func)
{
	m_weighters[ boost::algorithm::to_lower_copy( std::string(name))] = func;
}

const WeightingFunctionInterface* QueryProcessor::getWeightingFunction(
		const std::string& name) const
{
	std::map<std::string,const WeightingFunctionInterface*>::const_iterator 
		wi = m_weighters.find( boost::algorithm::to_lower_copy( std::string(name)));
	if (wi == m_weighters.end())
	{
		throw std::runtime_error( std::string( "weighting function not defined: '") + name + "'");
	}
	return wi->second;
}

void QueryProcessor::defineSummarizerFunction(
		const char* name,
		const SummarizerFunctionInterface* sumfunc)
{
	m_summarizers[ boost::algorithm::to_lower_copy( std::string(name))] = sumfunc;
}

const SummarizerFunctionInterface* QueryProcessor::getSummarizerFunction(
		const std::string& name) const
{
	std::map<std::string,const SummarizerFunctionInterface*>::const_iterator 
		si = m_summarizers.find( boost::algorithm::to_lower_copy( std::string(name)));
	if (si == m_summarizers.end())
	{
		throw std::runtime_error( std::string( "summarization function not defined: '") + name + "'");
	}
	return si->second;
}



