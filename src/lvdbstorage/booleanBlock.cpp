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

const char* BooleanBlock::find( const Index& elemno_, const char* lowerbound) const
{
	const char* itr = upper_bound( elemno_, lowerbound);
	if (!itr) return 0;
	Index from_;
	Index to_;
	char const* ii = itr;
	if (!getNextRange( ii, from_, to_)) return 0;
	return (from_ <= elemno_ && to_ >= elemno_)?itr:0;
}

const char* BooleanBlock::upper_bound( const Index& elemno_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return 0;
	if (id() < elemno_) throw std::logic_error("called BooleanBlock::upper_bound with wrong block");
	return findRangeIndexDesc( lowerbound, charend(), relativeIndexFromElemno( elemno_));
}

bool BooleanBlock::getNextRange( char const*& itr, Index& from_, Index& to_) const
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

	return getNextRange( pr, from_, to_);
}

void BooleanBlock::defineElement( const Index& elemno)
{
	defineRange( elemno, 0);
}


bool BooleanBlock::joinRange( Index& from_, Index& to_, const Index& from2_, const Index& to2_)
{
	if (from_ <= from2_)
	{
		if (to_ >= from2_)
		{
			if (to_ <= to2_)
			{
				//... case [from_][from2_][to_][to2_]
				to_ = to2_;
				return true;
			}
			else
			{
				//... case [from_][from2_][to2_][to_]
				// => first range covers 2nd => nothing to do
				return true;
			}
		}
		else
		{
			//...[from_][to_][from2_][to2_]
			if (to_+1 == from2_)
			{
				to_ = to2_;
				return true;
			}
			else
			{
				//...[from_][to_] ... [from2_][to2_] not joinable to one
				return false;
			}
		}
	}
	else
	{
		//...[from2_][from_] => swap szenario
		if (to2_ >= from_)
		{
			if (to2_ <= to_)
			{
				//... case [from2_][from_][to2_][to_]
				from_ = from2_;
				return true;
			}
			else
			{
				//... case [from2_][from_][to_][to2_]
				from_ = from2_;
				to_ = to2_;
				return true;
			}
		}
		else
		{
			//...[from2_][to2_][from_][to_]
			if (to2_+1 == from_)
			{
				from_ = from2_;
				return true;
			}
			else
			{
				//...[from2_][to2_] ... [from_][to_] not joinable to one
				return false;
			}
		}
	}
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


BooleanBlock BooleanBlockElementMap::merge( const_iterator ei, const const_iterator& ee, const BooleanBlock& oldblk)
{
	BooleanBlock rt( oldblk.blocktype());
	rt.setId( oldblk.id());

	char const* old_itr = oldblk.charptr();
	Index old_from;
	Index old_to;
	bool old_haselem = oldblk.getNextRange( old_itr, old_from, old_to);

	while (ei != ee && old_haselem)
	{
		if (ei->second)
		{
			//... define element
			if (BooleanBlock::joinRange( old_from, old_to, ei->first, ei->first))
			{
				++ei;
			}
			else
			{
				if (ei->first < old_from)
				{
					rt.defineElement( ei->first);
					++ei;
				}
				else
				{
					rt.defineRange( old_from, old_to - old_from);
					old_haselem = oldblk.getNextRange( old_itr, old_from, old_to);
				}
			}
		}
		else
		{
			//... delete element
			if (old_from <= ei->first)
			{
				if (old_to >= ei->first)
				{
					// .... that is inside the current range
					if (old_from < ei->first)
					{
						rt.defineRange( old_from, ei->first - old_from - 1);
					}
					old_from = ei->first + 1;
					++ei;
				}
				else
				{
					// .... that does not touch the old block
					rt.defineRange( old_from, old_to - old_from);
					old_haselem = oldblk.getNextRange( old_itr, old_from, old_to);
				}
			}
			else
			{
				//... delete undefined element => ignore
				++ei;
			}
		}
	}
	while (ei != ee)
	{
		if (ei->second)
		{
			rt.defineElement( ei->first);
		}
		++ei;
	}
	while (old_haselem)
	{
		rt.defineRange( old_from, old_to - old_from);
		old_haselem = oldblk.getNextRange( old_itr, old_from, old_to);
	}
	return rt;
}


