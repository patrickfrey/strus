/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "postingIteratorHelpers.hpp"

using namespace strus;

bool strus::callSkipDoc( strus::Index docno, PostingIteratorInterface** ar, std::size_t arsize, PostingIteratorInterface** valid_ar)
{
	bool rt = false;
	for (std::size_t ai=0; ai < arsize; ++ai)
	{
		if (docno == ar[ ai]->skipDoc( docno))
		{
			valid_ar[ ai] = ar[ ai];
			rt = true;
		}
		else
		{
			valid_ar[ ai] = 0;
		}
	}
	return rt;
}

Index strus::callSkipPos( strus::Index start, PostingIteratorInterface** ar, std::size_t size)
{
	Index rt = 0;
	std::size_t ti=0;
	for (; ti<size; ++ti)
	{
		if (ar[ ti])
		{
			Index pos = ar[ ti]->skipPos( start);
			if (pos)
			{
				if (!rt || pos < rt) rt = pos;
			}
		}
	}
	return rt;
}

std::pair<Index,Index> strus::callSkipPosWithLen( strus::Index start, PostingIteratorInterface** ar, std::size_t size)
{
	std::pair<Index,Index> rt( 0, 0);
	std::size_t ti=0;
	for (; ti<size; ++ti)
	{
		if (ar[ ti])
		{
			Index pos = ar[ ti]->skipPos( start);
			if (pos)
			{
				if (!rt.first)
				{
					rt = std::pair<Index,Index>( pos, ar[ ti]->length());
				}
				else if (pos <= rt.first)
				{
					if (pos < rt.first)
					{
						rt = std::pair<Index,Index>( pos, ar[ ti]->length());
					}
					else
					{
						rt = std::pair<Index,Index>( pos, std::max( rt.second, ar[ ti]->length()));
					}
				}
			}
		}
	}
	return rt;
}

