/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

