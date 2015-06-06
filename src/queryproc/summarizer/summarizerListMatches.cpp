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
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <set>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace strus;

void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name, const std::string&)
{
	throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "ListMatches", name.c_str());
}

void SummarizerFunctionInstanceListMatches::addNumericParameter( const std::string& name, const ArithmeticVariant&)
{
	throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "ListMatches", name.c_str());
}

void SummarizerExecutionContextListMatches::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_itrs.push_back( itr);
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization feature '%s'"), "ListMatches", name.c_str());
	}
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

std::vector<SummarizerExecutionContextInterface::SummaryElement>
	SummarizerExecutionContextListMatches::getSummary( const Index& docno)
{
	std::vector<SummaryElement> rt;
	std::vector<PostingIteratorInterface*>::const_iterator
		ii = m_itrs.begin(), ie = m_itrs.end();

	for (; ii != ie; ++ii)
	{
		std::vector<const PostingIteratorInterface*>
			subexpr = (*ii)->subExpressions( true);
		if ((*ii)->skipDoc( docno) != 0 && (*ii)->skipPos( 0) != 0)
		{
			rt.push_back( SummaryElement( getMatches( **ii, subexpr)));
		}
	}
	return rt;
}

