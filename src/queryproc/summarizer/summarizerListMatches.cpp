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
		float /*weight*/)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_itrs.push_back( itr);
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "ListMatches", name.c_str());
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding summarization feature to 'matchpos' summarizer: %s"), *m_errorhnd);
}

static std::string getMatches(
	PostingIteratorInterface& itr,
	const std::vector<const PostingIteratorInterface*>& subexpr)
{
	std::ostringstream rt;
	std::set<Index> mposet;
	std::vector<const PostingIteratorInterface*>::const_iterator si = subexpr.begin(), se = subexpr.end();
	for (; si != se; ++si)
	{
		mposet.insert( (*si)->posno());
	}
	mposet.insert( itr.posno());
	std::set<Index>::const_iterator ci = mposet.begin(), ce = mposet.end();
	for (int cidx=0; ci != ce; ++ci,++cidx)
	{
		if (cidx) rt << ' '; 
		rt << *ci;
	}
	return rt.str();
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
			std::vector<const PostingIteratorInterface*>
				subexpr = (*ii)->subExpressions( true);
			if ((*ii)->skipDoc( docno) == docno)
			{
				unsigned int kk=0;
				Index pos = (*ii)->skipPos( 0);
				for (; pos && kk<m_maxNofMatches; ++kk,pos = (*ii)->skipPos( pos+1))
				{
					rt.push_back( SummaryElement( getMatches( **ii, subexpr)));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error fetching 'matchpos' summary: %s"), *m_errorhnd, std::vector<SummarizerFunctionContextInterface::SummaryElement>());
}

void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name, const std::string&)
{
	m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "ListMatches", name.c_str());
}

void SummarizerFunctionInstanceListMatches::addNumericParameter( const std::string& name, const ArithmeticVariant& val)
{
	if (utils::caseInsensitiveEquals( name, "N"))
	{
		m_maxNofMatches = val.touint();
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "ListMatches", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating context of 'matchpos' summarizer: %s"), *m_errorhnd, 0);
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
	CATCH_ERROR_MAP_RETURN( _TXT("error creating instance of 'matchpos' summarizer: %s"), *m_errorhnd, 0);
}


