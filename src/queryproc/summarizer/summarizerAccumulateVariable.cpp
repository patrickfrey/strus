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
#include "summarizerAccumulateVariable.hpp"
#include "postingIteratorLink.hpp"
#include "ranker.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "private/bitOperations.hpp"
#include "private/localStructAllocator.hpp"
#include <cstdlib>

using namespace strus;

SummarizerFunctionContextAccumulateVariable::SummarizerFunctionContextAccumulateVariable(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		const std::string& var_,
		unsigned int maxNofElements_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_var(var_)
	,m_maxNofElements(maxNofElements_)
	,m_features()
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
	if (m_type.empty()) throw strus::runtime_error(_TXT("type of forward index to extract not defined (parameter 'type')"));
	if (m_var.empty()) throw strus::runtime_error(_TXT("variable to extract not defined (parameter 'var')"));
}


void SummarizerFunctionContextAccumulateVariable::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		float weight,
		const TermStatistics&)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			std::vector<const PostingIteratorInterface*> varitr;
			std::vector<SummarizationVariable>::const_iterator vi = variables.begin(), ve = variables.end();
			for (; vi != ve; ++vi)
			{
				if (utils::caseInsensitiveEquals( vi->name(), m_var))
				{
					varitr.push_back( vi->itr());
				}
			}
			if (m_features.size() >= 64)
			{
				m_errorhnd->report( _TXT("too many features (>64) defined for '%s' summarization"), "accuvariable");
			}
			else
			{
				if (varitr.empty())
				{
					m_errorhnd->report( _TXT("no variables with name '%s' defined in feature passed to '%s'"), m_var.c_str(), "accuvariable");
				}
				else
				{
					m_features.push_back( SummarizationFeature( itr, varitr, weight));
				}
			}
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "accuvariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), "accuvariable", *m_errorhnd);
}


static void setSelected( uint64_t& set, unsigned int idx)
{
	set |= ((uint64_t)1 << idx);
}

static int nextSelected( uint64_t& set)
{
	int rt = BitOperations::bitScanForward( set)-1;
	if (rt >= 0)
	{
		set -= (uint64_t)1<<rt;
	}
	return rt;
}

std::vector<SummarizerFunctionContextInterface::SummaryElement>
	SummarizerFunctionContextAccumulateVariable::getSummary( const Index& docno)
{
	try
	{
		typedef LocalStructAllocator<std::pair<Index,double> > PosWeightAllocator;
		typedef std::map<Index,double,std::less<Index>,PosWeightAllocator> PosWeightMap;

		uint64_t docsel = 0;

		m_forwardindex->skipDoc( docno);

		// Build a bitmap with all matching documents:
		std::vector<SummarizationFeature>::const_iterator
			fi = m_features.begin(), fe = m_features.end();
		unsigned int fidx=0;
		for (; fi != fe; ++fi,++fidx)
		{
			if (docno==fi->itr->skipDocCandidate( docno))
			{
				setSelected( docsel, fidx);
			}
		}
		// For every match position multiply the weights for each position and add them 
		// to the final accumulation result:
		PosWeightMap posWeightMap;
		uint64_t docselitr = docsel;
		int idx = 0;
		while (0<=(idx=nextSelected( docselitr)))
		{
			const SummarizationFeature& sumfeat = m_features[ idx];
			Index curpos = sumfeat.itr->skipPos( 0);
			for (; curpos; curpos = sumfeat.itr->skipPos( curpos+1))
			{
				PosWeightMap::iterator wi = posWeightMap.find( curpos);
				if (wi == posWeightMap.end())
				{
					posWeightMap[ curpos] = sumfeat.weight;
				}
				else
				{
					wi->second *= sumfeat.weight;
				}
			}
		}
		// Build the accumulation result:
		std::vector<SummarizerFunctionContextInterface::SummaryElement> rt;
		PosWeightMap::const_iterator wi = posWeightMap.begin(), we = posWeightMap.end();
		if (m_maxNofElements < posWeightMap.size())
		{
			Ranker ranker( m_maxNofElements);
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
					rt.push_back( SummarizerFunctionContextInterface::SummaryElement(
							m_forwardindex->fetch(), ri->weight));
				}
			}
		}
		else
		{
			for (; wi != we; ++wi)
			{
				if (m_forwardindex->skipPos( wi->first) == wi->first)
				{
					rt.push_back( SummarizerFunctionContextInterface::SummaryElement(
							m_forwardindex->fetch(), wi->second));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "accuvariable", *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}


void SummarizerFunctionInstanceAccumulateVariable::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "accuvariable");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "var"))
		{
			m_var = value;
		}
		else if (utils::caseInsensitiveEquals( name, "nof"))
		{
			m_errorhnd->report( _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "accuvariable");
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "accuvariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "accuvariable", *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateVariable::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "accuvariable");
	}
	else if (utils::caseInsensitiveEquals( name, "type")
	||  utils::caseInsensitiveEquals( name, "var"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "accuvariable");
	}
	else if (utils::caseInsensitiveEquals( name, "nof"))
	{
		m_maxNofElements = value.touint();
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "accuvariable", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateVariable::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextAccumulateVariable( storage, m_processor, m_type, m_var, m_maxNofElements, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "accuvariable", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceAccumulateVariable::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', var='" << m_var << "', nof="
			<< m_maxNofElements;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "accuvariable", *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateVariable::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateVariable( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "accuvariable", *m_errorhnd, 0);
}


SummarizerFunctionInterface::Description SummarizerFunctionAccumulateVariable::getDescription() const
{
	try
	{
		Description rt( _TXT("Accumulate the weights of all contents of a variable in matching expressions. Weights with same positions are grouped and multiplied, the group results are added to the sum, the total weight assigned to the variable content."));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features to inspect for variable matches"));
		rt( Description::Param::String, "type", _TXT( "the forward index feature type for the content to extract"));
		rt( Description::Param::String, "var", _TXT( "the name of the variable referencing the content to weight"));
		rt( Description::Param::String, "nof", _TXT( "the maximum number of the best weighted elements  to return (default 10)"));
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "accuvariable", *m_errorhnd, SummarizerFunctionInterface::Description());
}

