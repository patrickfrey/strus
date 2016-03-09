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
/// \brief Implementation of helper functions shared by iterators
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>

using namespace strus;

void strus::encodeInteger( std::string& buf, int val)
{
	std::ostringstream num;
	num << val;
	buf.append( num.str());
}

struct IteratorDf
{
	Index argidx;
	Index df;

	IteratorDf( const Index& argidx_, const Index& df_)
		:argidx(argidx_),df(df_){}
	IteratorDf( const IteratorDf& o)
		:argidx(o.argidx),df(o.df){}

	bool operator<( const IteratorDf& o) const
	{
		if (df == o.df) return argidx < o.argidx;
		return df > o.df;
	}
};

std::vector<PostingIteratorReference>
	strus::orderByDocumentFrequency(
		std::vector<PostingIteratorReference>::const_iterator ai,
		const std::vector<PostingIteratorReference>::const_iterator& ae)
{
	std::vector<IteratorDf> dfar;
	std::vector<PostingIteratorReference>::const_iterator start = ai;
	for (Index aidx=0; ai != ae; ++ai,++aidx)
	{
		dfar.push_back( IteratorDf( aidx, (*ai)->documentFrequency()));
	}
	std::sort( dfar.begin(), dfar.end());

	std::vector<Reference< PostingIteratorInterface> > rt;
	rt.reserve( dfar.size());
	std::vector<IteratorDf>::const_iterator di = dfar.begin(), de = dfar.end();
	for (; di != de; ++di)
	{
		rt.push_back( *(start + di->argidx));
	}
	return rt;
}

Index strus::minDocumentFrequency( const std::vector<PostingIteratorReference>& ar)
{
	std::vector<PostingIteratorReference>::const_iterator
		ai = ar.begin(), ae = ar.end();
	if (ai == ae) return 0;

	Index rt = (*ai)->documentFrequency();
	for (++ai; ai != ae; ++ai)
	{
		Index df = (*ai)->documentFrequency();
		if (df < rt)
		{
			rt = df;
		}
	}
	return rt;
}

Index strus::maxDocumentFrequency( const std::vector<PostingIteratorReference>& ar)
{
	std::vector<PostingIteratorReference>::const_iterator
		ai = ar.begin(), ae = ar.end();
	if (ai == ae) return 0;

	Index rt = (*ai)->documentFrequency();
	for (++ai; ai != ae; ++ai)
	{
		Index df = (*ai)->documentFrequency();
		if (df > rt)
		{
			rt = df;
		}
	}
	return rt;
}

