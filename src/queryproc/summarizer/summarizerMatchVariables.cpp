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
#include "summarizerMatchVariables.hpp"
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
#include <cstdlib>
/*[-]*/#include <sstream>
/*[-]*/#include <iostream>

using namespace strus;

SummarizerFunctionContextMatchVariables::SummarizerFunctionContextMatchVariables(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		const std::string& delimiter_,
		const std::string& assign_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_delimiter(delimiter_)
	,m_assign(assign_)
	,m_features()
	,m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get()) throw strus::runtime_error(_TXT("error creating forward index iterator"));
}


void SummarizerFunctionContextMatchVariables::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		float weight)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_features.push_back( SummarizationFeature( itr, variables, weight));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "MatchVariables", name.c_str());
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding feature to 'matchvariables' summarizer: %s"), *m_errorhnd);
}


std::vector<SummarizerFunctionContextInterface::SummaryElement>
	SummarizerFunctionContextMatchVariables::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummarizerFunctionContextInterface::SummaryElement> rt;
		m_forwardindex->skipDoc( docno);
		Index curpos = 0;

		std::vector<SummarizationFeature>::const_iterator
			fi = m_features.begin(), fe = m_features.end();

		for (; fi != fe; ++fi)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				curpos = fi->itr->skipPos( 0);
				for (; curpos; curpos = fi->itr->skipPos( curpos+1))
				{
					std::vector<SummarizationVariable>::const_iterator
						vi = fi->variables.begin(),
						ve = fi->variables.end();

					std::string line;
					for (int vidx=0; vi != ve; ++vi,++vidx)
					{
						Index pos = vi->position();
						if (pos)
						{
							if (vidx) line.append( m_delimiter);
							if (!m_assign.empty())
							{
								line.append( vi->name());
								line.append( m_assign);
							}
							m_forwardindex->skipPos( pos);
							line.append( m_forwardindex->fetch());
						}
					}
					rt.push_back( SummarizerFunctionContextInterface::SummaryElement( line, fi->weight));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching 'matchvariables' summary: %s"), *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}


void SummarizerFunctionInstanceMatchVariables::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "type"))
		{
			m_type = value;
		}
		else if (utils::caseInsensitiveEquals( name, "delimiter"))
		{
			m_delimiter = value;
		}
		else if (utils::caseInsensitiveEquals( name, "assign"))
		{
			m_assign = value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchVariables", name.c_str());
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding string parameter to 'matchvariables' summarizer: %s"), *m_errorhnd);
}

void SummarizerFunctionInstanceMatchVariables::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "type")
	||  utils::caseInsensitiveEquals( name, "delimiter")
	||  utils::caseInsensitiveEquals( name, "assign"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchVariables");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchVariables", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceMatchVariables::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*) const
{
	if (m_type.empty())
	{
		m_errorhnd->report( _TXT( "empty forward index type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		return new SummarizerFunctionContextMatchVariables( storage, m_processor, m_type, m_delimiter, m_assign, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating context of 'matchvariables' summarizer: %s"), *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMatchVariables::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', delimiter='" << m_delimiter
			<< "', assign='" << m_assign << "'";
		return rt.str();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error mapping 'matchvariables' summarizer to string: %s"), *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMatchVariables::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchVariables( processor, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating instance of 'matchvariables' summarizer: %s"), *m_errorhnd, 0);
}



