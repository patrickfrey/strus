/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "forwardIndexBlock.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include "private/cstring.h"
#include <cstring>

using namespace strus;

enum {EndItemMarker=(char)0xFE};

Index ForwardIndexBlock::position_at( const char* ref) const
{
	if (ref == charend()) return 0;
	char const* ri = ref;
	return positionFromRelativeIndex( unpackIndex( ri, charend()));
}

std::string ForwardIndexBlock::value_at( const char* ref) const
{
	if (ref == charend()) return 0;
	const char* namestart = skipIndex( ref, charend());
	const char* nameend = (const char*)std::memchr( namestart, EndItemMarker, charend()-namestart);
	if (!nameend) nameend = charend();
	return std::string( namestart, nameend - namestart);
}

const char* ForwardIndexBlock::nextItem( const char* ref) const
{
	if (ref == charend()) return 0;
	if (ref < charptr() || ref > charend()) throw strus::logic_error( _TXT( "illegal forward index block access (%s)"), __FUNCTION__);
	char const* rt = (const char*)std::memchr( ref, EndItemMarker, charend()-ref);
	return (rt)?(rt+1):charend();
}

const char* ForwardIndexBlock::prevItem( const char* ref) const
{
	if (ref == charptr()) return 0;
	if (ref < charptr() || ref > charend()) throw strus::logic_error( _TXT( "illegal forward index block access (%s)"), __FUNCTION__);
	char const* rt = (const char*)strus_memrchr( charptr(), EndItemMarker, ref-charptr()-1);
	return (rt >= charptr())?(rt+1):charptr();
}

const char* ForwardIndexBlock::upper_bound( const Index& pos_, const char* lowerbound) const
{
	if (!lowerbound || lowerbound == charend()) return 0;

	if (id() < pos_) throw strus::logic_error( _TXT( "called %s with wrong block"), __FUNCTION__);

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
	char const* pp = prevItem( charend());
	if (pp && position_at( pp) >= pos)
	{
		throw strus::runtime_error( _TXT( "forward index items not added in strictly ascending position order: %u after %u"), pos, position_at( pp));
	}
	if (id() < pos)
	{
		throw strus::runtime_error( _TXT( "upper bound of position in forward index block not set (%s)"), __FUNCTION__);
	}
	std::string blk;
	if (size()) blk.push_back( EndItemMarker);
	packIndex( blk, relativeIndexFromPosition( pos));	//... relative position
	blk.append( item);

	DataBlock::append( blk.c_str(), blk.size());
}

void ForwardIndexBlock::setId( const Index& id_)
{
	if (id() == id_) return;
	if (empty())
	{
		DataBlock::setId( id_);
	}
	else
	{
		char const* pp = prevItem( charend());
		Index maxPos = position_at( pp);
		
		if (maxPos > id_) throw strus::runtime_error( "%s", _TXT( "cannot set forward index block id to a smaller value than the highest position of an item inserted"));
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
				if (!bi) break;
				content.append( blkstart, bi - blkstart);
			}
			init( id_, content.c_str(), content.size(), content.size());
		}
	}
}


