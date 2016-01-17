/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "summarizerListMatches.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <set>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace strus;

void SummarizerFunctionContextListMatches::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		float /*weight*/,
		const TermStatistics&)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_itrs.push_back( itr);
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "matchpos", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding summarization feature to '%s' summarizer: %s"), "matchpos", *m_errorhnd);
}

std::vector<SummarizerFunctionContextInterface::SummaryElement>
	SummarizerFunctionContextListMatches::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		std::vector<PostingIteratorInterface*>::const_iterator
			ii = m_itrs.begin(), ie = m_itrs.end();
	
		for (; ii != ie; ++ii)
		{
			if ((*ii)->skipDoc( docno) == docno)
			{
				unsigned int kk=0;
				Index pos = (*ii)->skipPos( 0);
				for (; pos && kk<m_maxNofMatches; ++kk,pos = (*ii)->skipPos( pos+1))
				{
					char posstr[ 64];
					snprintf( posstr, sizeof(posstr), "%u", (unsigned int)pos);
					rt.push_back( SummaryElement( posstr));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "matchpos", *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}

void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name, const std::string&)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchpos");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "matchpos", name.c_str());
	}
}

void SummarizerFunctionInstanceListMatches::addNumericParameter( const std::string& name, const ArithmeticVariant& val)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "matchpos");
	}
	else if (utils::caseInsensitiveEquals( name, "N"))
	{
		m_maxNofMatches = val.touint();
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "matchpos", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "matchpos", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceListMatches::tostring() const
{
	return std::string();
}


SummarizerFunctionInstanceInterface* SummarizerFunctionListMatches::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceListMatches( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "matchpos", *m_errorhnd, 0);
}


SummarizerFunctionInterface::Description SummarizerFunctionListMatches::getDescription() const
{
	try
	{
		Description rt( _TXT("Get the feature occurencies printed"));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features"));
		rt( Description::Param::Numeric, "N", _TXT( "the maximum number of matches to return"));
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "matchpos", *m_errorhnd, SummarizerFunctionInterface::Description());
}


