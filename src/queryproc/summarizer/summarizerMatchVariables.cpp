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
#include "private/utils.hpp"
#include <cstdlib>

using namespace strus;

void SummarizerFunctionInstanceMatchVariables::addParameter( const std::string& name, const std::string& value)
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

void SummarizerFunctionInstanceMatchVariables::addParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "type")
	||  utils::caseInsensitiveEquals( name, "delimiter")
	||  utils::caseInsensitiveEquals( name, "assign"))
	{
		throw strus::runtime_error( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MatchVariables");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MatchVariables", name.c_str());
	}
}


SummarizerClosureMatchVariables::SummarizerClosureMatchVariables(
		const StorageClientInterface* storage_,
		const QueryProcessorInterface* processor_,
		const std::string& type_,
		const std::string& delimiter_,
		const std::string& assign_)
	:m_storage(storage_)
	,m_processor(processor_)
	,m_forwardindex(storage_->createForwardIterator( type_))
	,m_type(type_)
	,m_delimiter(delimiter_.empty()?std::string(","):delimiter_)
	,m_assign(assign_.empty()?std::string("="):assign_)
	,m_features()
{}


void SummarizerClosureMatchVariables::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_features.push_back( SummarizationFeature( itr, variables));
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization feature '%s'"), "MatchVariables", name.c_str());
	}
}


std::vector<SummarizerClosureInterface::SummaryElement>
	SummarizerClosureMatchVariables::getSummary( const Index& docno)
{
	std::vector<SummarizerClosureInterface::SummaryElement> rt;
	m_forwardindex->skipDoc( docno);
	Index curpos = 0;

	std::vector<SummarizationFeature>::const_iterator
		fi = m_features.begin(), fe = m_features.end();

	for (; fi != fe; ++fi)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			for (curpos = 0; curpos; curpos=fi->itr->skipPos( curpos+1))
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
						line.append( vi->name());
						line.append( m_assign);
						m_forwardindex->skipPos( pos);
						line.append( m_forwardindex->fetch());
					}
				}
			}
		}
	}
	return rt;
}


