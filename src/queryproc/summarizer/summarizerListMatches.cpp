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
#include "strus/iteratorInterface.hpp"
#include "strus/forwardIndexViewerInterface.hpp"
#include "strus/storageInterface.hpp"
#include <set>

using namespace strus;

SummarizerListMatches::SummarizerListMatches(
		StorageInterface* storage_,
		std::size_t nofitrs_,
		const IteratorInterface** itrs_)
	:m_storage(storage_)
{
	for (std::size_t ii=0; ii<nofitrs_; ++ii)
	{
		if (itrs_[ii])
		{
			m_itr.push_back( itrs_[ii]->copy());
		}
	}
}

SummarizerListMatches::~SummarizerListMatches()
{}

static std::string getMatches(
	IteratorInterface& itr,
	const std::vector<IteratorInterface*>& subexpr)
{
	std::ostringstream rt;
	std::set<Index> mposet;
	std::vector<IteratorInterface*>::const_iterator si = subexpr.begin(), se = subexpr.end();
	for (; si != se; ++si)
	{
		mposet.insert( (*si)->posno());
	}
	mposet.insert( itr.posno());
	std::set<Index>::const_reverse_iterator ci = mposet.rbegin(), ce = mposet.rend();
	for (int cidx=0; ci != ce; ++ci,++cidx)
	{
		if (cidx) rt << ' '; 
		rt << *ci;
	}
	return rt.str();
}

std::vector<std::string>
	SummarizerListMatches::getSummary( const Index& docno)
{
	std::vector<std::string> rt;
	std::vector<IteratorReference>::const_iterator ii = m_itr.begin(), ie = m_itr.end();
	for (; ii != ie; ++ii)
	{
		std::vector<IteratorInterface*> subexpr = (*ii)->subExpressions( true);
		if ((*ii)->skipDoc( docno) != 0 && (*ii)->skipPos( docno) != 0)
		{
			rt.push_back( getMatches( **ii, subexpr));
		}
	}
	return rt;
}
