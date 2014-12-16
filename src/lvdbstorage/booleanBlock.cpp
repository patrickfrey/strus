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
#include "booleanBlock.hpp"
#include "indexPacker.hpp"
#include <limits>

using namespace strus;

const char* BooleanBlock::find( const Index& docno_, const char* lowerbound) const
{
	const char* itr = upper_bound( docno_, lowerbound);
	if (!itr) return 0;
	Index from_;
	Index to_;
	getRange( itr, from_, to_);
	return (from_ <= docno_ && to_ >= docno_)?itr:0;
}

const char* BooleanBlock::upper_bound( const Index& docno_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return 0;
	if (id() < docno_) throw std::logic_error("called BooleanBlock::upper_bound with wrong block");
	return findRangeIndexDesc( lowerbound, charend(), relativeIndexFromElemno( docno_));
}

bool BooleanBlock::getRange( const char* itr, Index& from_, Index& to_) const
{
	if (itr == charend()) return false;
	Index rangesize;
	Index top = unpackRange( itr, charend(), rangesize);
	to_ = elemnoFromRelativeIndex( top);
	from_ = to_ - rangesize;
	return true;
}

bool BooleanBlock::getLastRange( std::size_t& at_, Index& from_, Index& to_) const
{
	if (size() == 0) return false;

	char const* pr = prevPackedRangePos( charptr(), charend()-1);
	at_ = pr - charptr();

	return getRange( pr, from_, to_);
}

void BooleanBlock::defineElement( const Index& elemno)
{
	defineRange( elemno, 0);
}

void BooleanBlock::defineRange( const Index& elemno, const Index& rangesize)
{
	std::size_t at_;
	Index from_;
	Index to_;
	char buf[64];
	std::size_t bufpos = 0;

	if (getLastRange( at_, from_, to_))
	{
		if (elemno < from_)
		{
			throw std::logic_error( "ranges not appended in order in boolean block");
		}
		if (elemno <= to_ + 1)
		{
			//... overlapping ranges => join them
			if (to_ < elemno + rangesize)
			{
				to_ = elemno + rangesize;
				resize( at_);
				packRange( buf, bufpos, sizeof(buf), relativeIndexFromElemno( from_), to_);
				append( buf, bufpos);
			}
			else
			{
				//... new range inside old one
			}
		}
		else
		{
			//... not overlapping with last range => add new
			packRange( buf, bufpos, sizeof(buf), relativeIndexFromElemno( elemno), rangesize);
			append( buf, bufpos);
		}
	}
	else
	{
		//... first range => add new
		packRange( buf, bufpos, sizeof(buf), relativeIndexFromElemno( elemno), rangesize);
		append( buf, bufpos);
	}
}

BooleanBlock BooleanBlock::merge( const BooleanBlock& newblk, const BooleanBlock& oldblk)
{
	if (newblk.blocktype() != oldblk.blocktype()) throw std::logic_error("merging blocks of different types");
	BooleanBlock rt( newblk.blocktype());

//	char const* ni = newblk.charptr();
//	char const* ne = newblk.charend();
//	char const* oi = oldblk.charptr();
//	char const* oe = oldblk.charend();
}

