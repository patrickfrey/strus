/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_MEMBER_PACKED_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_MEMBER_PACKED_HPP_INCLUDED
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstring>

namespace strus {

template <int bytesize>
struct StructBlockMemberPackedData
{};

template <>
struct StructBlockMemberPackedData<1>
{
	typedef unsigned short PositionType;
	typedef unsigned short PackType;

	PositionType base;
	PackType ofs[ 7];
	//... NOTE: Packing is wasting 10 bits of first ofs that are redundant (equal size)

	enum {OfsMask=(1<<10)-1, OfsShift=10, MaxOfs=(1<<10)-1, MaxSize=(1<<6)-1, NofOfs=7};
};

template <>
struct StructBlockMemberPackedData<2>
{
	typedef unsigned short PositionType;
	typedef unsigned char PackType;

	PositionType base;
	PackType ofs[ 14];
	//... NOTE: Packing is wasting 5 bits of first ofs that are redundant (equal size)

	enum {OfsMask=(1<<5)-1, OfsShift=5, MaxOfs=(1<<5)-1, MaxSize=(1<<3)-1, NofOfs=14};
};


struct StructBlockMemberPackedShort
{
	typedef unsigned short PositionType;
	typedef unsigned short PackType;

	PositionType base;
	PackType ofs[ 7];
	//... NOTE: Packing is wasting 10 bits of first ofs that are redundant (equal size)

	enum {OfsMask=(1<<10)-1, OfsShift=10, MaxOfs=(1<<10)-1, MaxSize=(1<<6)-1, NofOfs=7};
	static PositionType nextofs( PackType pk) {return pk & (unsigned short)OfsMask;}
	static PositionType nextsize( PackType pk) {return pk >> (unsigned short)OfsShift;}

	bool check() const
	{
		int pi=0, pe=NofOfs;
		for (; pi!=pe && ofs[pi]; ++pi){}
		for (; pi!=pe && !ofs[pi]; ++pi){}
		return !!base && pi == pe && (ofs[0] & OfsMask) == 0;
	}

	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static PositionType offsetCast( PositionType ofs)
	{
		return (ofs <= (strus::Index)MaxOfs) ? (PositionType)ofs : 0;
	}
	static PositionType sizeCast( strus::Index sz)
	{
		return (sz <= (strus::Index)MaxSize) ? (PositionType)sz : 0;
	}
	PackType packedRange( PositionType prev, const strus::IndexRange& range)
	{
		if (range.start() < prev) throw std::runtime_error(_TXT("packed members not appended in ascending order"));
		PositionType endpos = positionCast( range.end());
		if (!endpos) throw std::runtime_error(_TXT("packed members position out of range"));
		PositionType relend = offsetCast( endpos - prev);
		PositionType diff = sizeCast( range.end() - range.start());
		if (!relend || !diff) return 0;
		return relend + (diff << OfsShift);
	}
	static PositionType getEndOfs( PackType pk)
	{
		return pk & OfsMask;
	}
	static PositionType getSize( PackType pk)
	{
		return pk >> OfsShift;
	}

	explicit StructBlockMemberPackedShort( PositionType base_=0)
		:base(base_)
	{
		std::memset( ofs, 0, sizeof(ofs));
	}

	StructBlockMemberPackedShort( const StructBlockMemberPackedShort& o)
		:base(o.base)
	{
		std::memcpy( ofs, o.ofs, sizeof(ofs));
	}

	bool full() const {return !!ofs[NofOfs-1];}

	bool append( const strus::IndexRange& range)
	{
		if (base == 0)
		{
			base = range.start();
			ofs[ 0] = packedRange( base, range);
			if (!ofs[ 0]) return false;
		}
		else
		{
			PositionType accu = base;
			int pi = 0, pe = NofOfs;
			for (; pi<pe && ofs[pi]; accu += getEndOfs(ofs[pi]),++pi){}
			if (pi == pe) return false;
			ofs[ pi] = packedRange( accu, range);
			if (!ofs[ pi]) return false;
		}
		return true;
	}

	PositionType end() const
	{
		PositionType rt=base;
		for (int ii=0; ii<=NofOfs; rt+=getEndOfs(ofs[ii]),++ii){}
		return rt;
	}

	strus::IndexRange skip( PositionType pos) const
	{
		PositionType rtend = base;
		int ii = 0;
		for (; ii<=NofOfs && rtend < pos; rtend+=getEndOfs(ofs[ii]),++ii){}
		if (rtend > pos) return strus::IndexRange( rtend - getSize( ofs[ii]), rtend);
		return strus::IndexRange();
	}
};

}//namespace
#endif


