/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerAccumulateVariable.hpp"
#include "postingIteratorLink.hpp"
#include "ranker.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define METHOD_NAME "accuvar"

SummarizerFunctionContextAccumulateVariable::SummarizerFunctionContextAccumulateVariable(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const Reference<AccumulateVariableData> data_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( data_->type))
	,m_data(data_)
	,m_features()
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error( "%s", _TXT("error creating forward index iterator"));
	if (m_data->type.empty()) throw strus::runtime_error( "%s", _TXT("type of forward index to extract not defined (parameter 'type')"));
	if (m_data->var.empty()) throw strus::runtime_error( "%s", _TXT("no variable to extract defined (parameter 'var')"));
}

void SummarizerFunctionContextAccumulateVariable::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseNotImplemented), _TXT("no variables known for function '%s'"), METHOD_NAME);
}

void SummarizerFunctionContextAccumulateVariable::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		double weight,
		const TermStatistics&)
{
	try
	{
		if (strus::caseInsensitiveEquals( name, "match"))
		{
			std::vector<const PostingIteratorInterface*> varitr;
			std::vector<SummarizationVariable>::const_iterator vi = variables.begin(), ve = variables.end();
			for (; vi != ve; ++vi)
			{
				if (strus::caseInsensitiveEquals( m_data->var, vi->name()))
				{
					varitr.push_back( vi->itr());
				}
			}
			if (varitr.empty())
			{
				m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no variables with name '%s' defined in feature passed to '%s'"), m_data->var.c_str(), METHOD_NAME);
			}
			if (m_features.size() >= MaxNofFeatures)
			{
				m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseMaximumLimitReached), _TXT("to many features defined for '%s'"), METHOD_NAME);
			}
			else
			{
				m_features.push_back( SummarizationFeature( itr, varitr, weight));
			}
		}
		else
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT("unknown '%s' summarization feature '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

std::vector<unsigned int> SummarizerFunctionContextAccumulateVariable::getCandidateSet( const Index& docno) const
{
	std::vector<unsigned int> rt;
	std::vector<SummarizationFeature>::const_iterator
		fi = m_features.begin(), fe = m_features.end();
	unsigned int fidx=0;
	for (; fi != fe; ++fi,++fidx)
	{
		if (docno==fi->itr->skipDocCandidate( docno))
		{
			rt.push_back( fidx);
		}
	}
	return rt;
}

SummarizerFunctionContextAccumulateVariable::PosWeightMap SummarizerFunctionContextAccumulateVariable::buildPosWeightMap( const std::vector<unsigned int>& fsel)
{
	PosWeightMap rt;

	std::vector<unsigned int>::const_iterator fi = fsel.begin(), fe = fsel.end();
	for (; fi != fe; ++fi)
	{
		const SummarizationFeature& sumfeat = m_features[ *fi];
		Index curpos = sumfeat.itr->skipPos( 0);
		for (; curpos; curpos = sumfeat.itr->skipPos( curpos+1))
		{
			std::vector<const PostingIteratorInterface*>::const_iterator
				vi = sumfeat.varitr.begin(), ve = sumfeat.varitr.end();
			for (;vi != ve; ++vi)
			{
				PosWeightMap::iterator wi = rt.find( curpos);
				if (wi == rt.end())
				{
					rt[ curpos] = sumfeat.weight;
				}
				else
				{
					wi->second *= m_data->cofactor * sumfeat.weight;
				}
			}
		}
	}
	return rt;
}

void SummarizerFunctionContextAccumulateVariable::printPosWeights( std::ostream& out, const std::vector<unsigned int>& fsel)
{
	PosWeightMap posWeightMap;

	std::vector<unsigned int>::const_iterator fi = fsel.begin(), fe = fsel.end();
	for (; fi != fe; ++fi)
	{
		const SummarizationFeature& sumfeat = m_features[ *fi];
		Index curpos = sumfeat.itr->skipPos( 0);
		for (; curpos; curpos = sumfeat.itr->skipPos( curpos+1))
		{
			std::vector<const PostingIteratorInterface*>::const_iterator
				vi = sumfeat.varitr.begin(), ve = sumfeat.varitr.end();
			for (;vi != ve; ++vi)
			{
				PosWeightMap::iterator wi = posWeightMap.find( curpos);
				if (wi == posWeightMap.end())
				{
					out << string_format( _TXT( "accu pos=%u, weight=%f"),
								(unsigned int)curpos, sumfeat.weight)
						<< std::endl;
				}
				else
				{
					wi->second *= m_data->cofactor * sumfeat.weight;
					out << string_format( _TXT( "accu pos=%u, weight=%f, result=%f"),
								(unsigned int)curpos,
								(m_data->cofactor * sumfeat.weight), wi->second)
						<< std::endl;
				}
			}
		}
	}
	std::vector<SummaryElement> res = getSummariesFromPosWeightMap( posWeightMap);
	std::vector<SummaryElement>::const_iterator ri = res.begin(), re = res.end();
	for (; ri != re; ++ri)
	{
		out << string_format( _TXT( "result key=%s, value=%s, weight=%f"), ri->name().c_str(), ri->value().c_str(), ri->weight());
	}
}

std::vector<SummaryElement> SummarizerFunctionContextAccumulateVariable::getSummariesFromPosWeightMap( const PosWeightMap& posWeightMap)
{
	std::vector<SummaryElement> rt;
	PosWeightMap::const_iterator wi = posWeightMap.begin(), we = posWeightMap.end();
	if (m_data->maxNofElements < posWeightMap.size())
	{
		Ranker ranker( m_data->maxNofElements);
		for (; wi != we; ++wi)
		{
			ranker.insert( wi->second, wi->first);
		}
		std::vector<Ranker::Element> ranklist = ranker.result();
		std::vector<Ranker::Element>::const_iterator ri = ranklist.begin(), re = ranklist.end();
		for (; ri != re; ++ri)
		{
			if (m_forwardindex->skipPos( ri->idx) == ri->idx)
			{
				rt.push_back( SummaryElement(
						m_data->resultname, m_forwardindex->fetch(),
						ri->weight * m_data->norm));
			}
		}
	}
	else
	{
		for (; wi != we; ++wi)
		{
			if (m_forwardindex->skipPos( wi->first) == wi->first)
			{
				rt.push_back( SummaryElement(
						m_data->resultname, m_forwardindex->fetch(),
						wi->second * m_data->norm));
			}
		}
	}
	return rt;
}

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateVariable::getSummary( const Index& docno)
{
	try
	{
		m_forwardindex->skipDoc( docno);

		// Build a bitmap with all matching documents:
		std::vector<unsigned int> flist = getCandidateSet( docno);

		// For every match position multiply the weights for each position and add them 
		// to the final accumulation result:
		PosWeightMap posWeightMap( buildPosWeightMap( flist));

		// Build the accumulation result:
		return getSummariesFromPosWeightMap( posWeightMap);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextAccumulateVariable::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), METHOD_NAME) << std::endl;

	m_forwardindex->skipDoc( docno);

	// Build a bitmap with all matching documents:
	std::vector<unsigned int> flist = getCandidateSet( docno);

	// Log events that contribute to the result:
	printPosWeights( out, flist);

	return out.str();
}


void SummarizerFunctionInstanceAccumulateVariable::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name, "type"))
		{
			m_data->type = value;
		}
		else if (strus::caseInsensitiveEquals( name, "var"))
		{
			m_data->var = value;
			if (m_data->resultname.empty())
			{
				m_data->resultname = value;
			}
		}
		else if (strus::caseInsensitiveEquals( name, "result"))
		{
			m_data->resultname = value;
		}
		else if (strus::caseInsensitiveEquals( name, "cofactor"))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name, "norm"))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name, "nof"))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateVariable::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name, "var"))
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name, "nof"))
	{
		m_data->maxNofElements = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name, "norm"))
	{
		m_data->norm = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name, "cofactor"))
	{
		m_data->cofactor = value.tofloat();
	}
	else
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

void SummarizerFunctionInstanceAccumulateVariable::defineResultName( const std::string& resultname, const std::string& itemname)
{
	try
	{
		throw strus::runtime_error(_TXT("no result rename defined for '%s' summarizer"), METHOD_NAME);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateVariable::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextAccumulateVariable( storage, m_processor, m_data, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceAccumulateVariable::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_data->type << "'";
		rt << ", nof=" << m_data->maxNofElements;
		rt << ", norm=" << m_data->norm;
		rt << ", cofactor=" << m_data->cofactor;
		rt << ", var=" << m_data->var;
		rt << ", result=" << m_data->resultname;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateVariable::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateVariable( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}


FunctionDescription SummarizerFunctionAccumulateVariable::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Accumulate the weights of all contents of a variable in matching expressions. Weights with same positions are grouped and multiplied, the group results are added to the sum, the total weight assigned to the variable content."));
		rt( P::Feature, "match", _TXT( "defines the query features to inspect for variable matches"), "");
		rt( P::String, "type", _TXT( "the forward index feature type for the content to extract"), "");
		rt( P::String, "var", _TXT( "the name of the variable referencing the content to weight"), "");
		rt( P::Numeric, "nof", _TXT( "the maximum number of the best weighted elements  to return (default 10)"), "1:");
		rt( P::Numeric, "norm", _TXT( "the normalization factor of the calculated weights (default 1.0)"), "0.0:1.0");
		rt( P::Numeric, "cofactor", _TXT( "additional multiplier for coincident matches (default 1.0)"), "0.0:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

