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
#include "forwardIndexBlock.hpp"
#include "indexPacker.hpp"
#include <cstring>
/*[-]*/ #include <iostream>

using namespace strus;

enum {EndItemMarker=(char)0xFE};

Index ForwardIndexBlock::position_at( const char* ref) const
{
	if (ref == charend()) return 0;
	char const* ri = ref;
	return positionFromRelativeIndex( unpackIndex( ri, charend()));
}

const char* ForwardIndexBlock::value_at( const char* ref) const
{
	if (ref == charend()) return 0;
	return skipIndex( ref, charend());
}

const char* ForwardIndexBlock::nextItem( const char* ref) const
{
	if (ref == charend()) return 0;
	if (ref < charptr() || ref > charend()) throw std::logic_error("illegal forward index block access -- nextItem");
	char const* rt = (const char*)std::memchr( ref, EndItemMarker, charend()-ref);
	return (rt)?(rt+1):charend();
}

const char* ForwardIndexBlock::prevItem( const char* ref) const
{
	if (ref == charptr()) return 0;
	if (ref < charptr() || ref > charend()) throw std::logic_error("illegal forward index block access -- prevItem");
	char const* rt = (const char*)::memrchr( charptr(), EndItemMarker, ref-charptr()-1);
	return (rt >= charptr())?(rt+1):charptr();
}

const char* ForwardIndexBlock::upper_bound( const Index& pos_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return 0;

	if (id() < pos_) throw std::logic_error("called ForwardIndexBlock::upper_bound with wrong block");

	const char* rt = findStructIndexDesc(
				lowerbound, charend(), EndItemMarker,
				relativeIndexFromPosition( pos_));
	return rt;
}

const char* ForwardIndexBlock::find( const Index& pos_, const char* lowerbound) const
{
	const char* rt = upper_bound( pos_, lowerbound);
	return (pos_ == position_at( rt))?rt:0;
}

void ForwardIndexBlock::append( const Index& pos, const std::string& item)
{
	/*[-]*/ std::cout << "Block append forward index pos " << pos << " item " << item << std::endl;

	char const* pp = prevItem( charend());
	if (pp && position_at( pp) >= pos)
	{
		throw std::runtime_error( "forward index items not added in strictly ascending position order");
	}
	if (id() < pos)
	{
		throw std::runtime_error( "internal: upper bound of position in forward index block not set (setId)");
	}
	std::string blk;
	if (size()) blk.push_back( EndItemMarker);
	packIndex( blk, relativeIndexFromPosition( pos));	//... relative docno
	blk.append( item);

	DataBlock::append( blk.c_str(), blk.size());
}

void ForwardIndexBlock::setId( const Index& id_)
{
	if (empty())
	{
		DataBlock::setId( id_);
	}
	else
	{
		char const* pp = prevItem( charend());
		Index maxPos = position_at( pp);
		
		if (maxPos > id_) throw std::runtime_error( "internal: cannot set forward index block id to a smaller value than the highest position of an item inserted");
		if (id() != id_)
		{
			// Rewrite relative position numbers (first element in variable size record):
			std::string content;
			Index id_diff = id_ - id();
			char const* bi = charptr();
			const char* be = charend();
			
			while (bi != be)
			{
				packIndex( content, unpackIndex( bi, be) + id_diff);
				const char* blkstart = bi;
				bi = nextItem( bi);
				content.append( blkstart, bi - blkstart);
			}
			init( id_, content.c_str(), content.size(), content.size());
		}
	}
}


