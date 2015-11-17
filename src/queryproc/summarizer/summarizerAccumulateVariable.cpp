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
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_var(var_)
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
		float weight)
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
				m_errorhnd->report( _TXT("too many features (>64) defined for '%s' summarization"), "AccumulateVariable");
			}
			else
			{
				if (varitr.empty())
				{
					m_errorhnd->report( _TXT("no variables with name '%s' defined in feature passed to '%s'"), m_var.c_str(), "AccumulateVariable");
				}
				else
				{
					m_features.push_back( SummarizationFeature( itr, varitr, weight));
				}
			}
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "AccumulateVariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), "AccumulateVariable", *m_errorhnd);
}


static void clearSelected( uint64_t& set, unsigned int idx)
{
	set &= ~((uint64_t)1 << idx);
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
		Index curpos = 0;

		// Build a bitmap with all matching documents:
		std::vector<SummarizationFeature>::const_iterator
			fi = m_features.begin(), fe = m_features.end();
		unsigned int fidx=0;
		for (; fi != fe; ++fi,++fidx)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				setSelected( docsel, fidx);
			}
		}
		// Find out first match position to start with:
		for (fidx = 0, fi = m_features.begin(); fi != fe; ++fi,++fidx)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				Index dn = fi->itr->skipPos( curpos);
				if (dn)
				{
					setSelected( docsel, fidx);
					if (curpos == 0 || dn < curpos)
					{
						curpos = dn;
					}
				}
			}
		}
		// For every match position multiply the weights for each position and add them 
		// to the final accumulation result:
		PosWeightMap totalmap;
		while (curpos)
		{
			Index nextpos = 0;
			PosWeightMap curmap;
			uint64_t docselitr = docsel;
			int idx = 0;
			while (0<=(idx=nextSelected( docselitr)))
			{
				const SummarizationFeature& sumfeat = m_features[ idx];
				Index np = sumfeat.itr->skipPos( curpos);
				if (!np)
				{
					clearSelected( docsel, idx);
					continue;
				}
				if (np == curpos)
				{
					std::vector<const PostingIteratorInterface*>::const_iterator
						vi = sumfeat.varitr.begin(), ve = sumfeat.varitr.end();
					for (; vi != ve; ++vi)
					{
						Index pos = (*vi)->posno();
						PosWeightMap::iterator mi = curmap.find( pos);
						if (mi == curmap.end())
						{
							curmap[ pos] = sumfeat.weight;
						}
						else
						{
							curmap[ pos] *= sumfeat.weight;
						}
					}
					if (!nextpos)
					{
						nextpos = sumfeat.itr->skipPos( curpos+1);
					}
				}
				else if (nextpos == 0 || np < nextpos)
				{
					nextpos = np;
				}
			}
			PosWeightMap::const_iterator ci = curmap.begin(), ce = curmap.end();
			for (; ci != ce; ++ci)
			{
				totalmap[ ci->first] += ci->second;
			}
			curpos = nextpos;
		}

		// Build the accumulation result:
		std::vector<SummarizerFunctionContextInterface::SummaryElement> rt;
		PosWeightMap::const_iterator ti = totalmap.begin(), te = totalmap.end();
		for (; ti != te; ++ti)
		{
			if (m_forwardindex->skipPos( ti->first) == ti->first)
			{
				rt.push_back( SummarizerFunctionContextInterface::SummaryElement(
						m_forwardindex->fetch(), ti->second));
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "AccumulateVariable", *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}


void SummarizerFunctionInstanceAccumulateVariable::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "AccumulateVariable");
		}
		else if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "var"))
		{
			m_var = value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "AccumulateVariable", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "AccumulateVariable", *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateVariable::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "AccumulateVariable");
	}
	else if (utils::caseInsensitiveEquals( name, "type")
	||  utils::caseInsensitiveEquals( name, "var"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "AccumulateVariable");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "AccumulateVariable", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateVariable::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextAccumulateVariable( storage, m_processor, m_type, m_var, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "AccumulateVariable", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceAccumulateVariable::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', var='" << m_var << "'";
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "AccumulateVariable", *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateVariable::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateVariable( processor, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "AccumulateVariable", *m_errorhnd, 0);
}


