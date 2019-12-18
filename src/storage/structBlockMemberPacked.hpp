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
class StructBlockMemberPackedData<1>
{
public:
	typedef unsigned short PositionType;
	typedef unsigned short PackType;

	PositionType base;
	PackType ofs[ 7];
	//... NOTE: Packing is wasting 10 bits of first ofs that are redundant (equal size)

	enum {OfsMask=(1<<10)-1, OfsShift=10, MaxOfs=(1<<10)-1, MaxSize=(1<<6)-1, NofOfs=7};
};

template <>
class StructBlockMemberPackedData<2>
{
public:
	typedef unsigned short PositionType;
	typedef unsigned char PackType;

	PositionType base;
	PackType ofs[ 14];
	//... NOTE: Packing is wasting 5 bits of first ofs that are redundant (equal size)

	enum {OfsMask=(1<<5)-1, OfsShift=5, MaxOfs=(1<<5)-1, MaxSize=(1<<3)-1, NofOfs=14};
};


template <int bytesize>
class StructBlockMemberPacked
	:public StructBlockMemberPackedData<bytesize>
{
public:
	typedef StructBlockMemberPackedData<bytesize> Parent;
	typedef typename Parent::PositionType PositionType;
	typedef typename Parent::PackType PackType;
	enum {OfsMask=Parent::OfsMask, OfsShift=Parent::OfsShift, MaxOfs=Parent::MaxOfs, MaxSize=Parent::MaxSize, NofOfs=Parent::NofOfs};

	static PositionType nextofs( PackType pk) {return pk & (unsigned short)OfsMask;}
	static PositionType nextsize( PackType pk) {return pk >> (unsigned short)OfsShift;}

	bool check() const
	{
		int pi=0, pe=NofOfs;
		for (; pi!=pe && Parent::ofs[pi]; ++pi){}
		for (; pi!=pe && !Parent::ofs[pi]; ++pi){}
		return !!Parent::base && pi == pe && (Parent::ofs[0] & OfsMask) == 0;
	}

	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static PositionType offsetCast( PositionType diff)
	{
		return (diff <= (strus::Index)MaxOfs) ? (PositionType)diff : 0;
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

	explicit StructBlockMemberPacked( PositionType base_=0)
	{
		Parent::base = base_;
		std::memset( Parent::ofs, 0, sizeof(Parent::ofs));
	}

	StructBlockMemberPacked( const StructBlockMemberPacked& o)
	{
		Parent::base = o.base;
		std::memcpy( Parent::ofs, o.ofs, sizeof(Parent::ofs));
	}

	bool full() const {return !!Parent::ofs[NofOfs-1];}

	bool append( const strus::IndexRange& range)
	{
		if (Parent::base == 0)
		{
			Parent::base = range.start();
			Parent::ofs[ 0] = packedRange( Parent::base, range);
			if (!Parent::ofs[ 0]) return false;
		}
		else
		{
			PositionType accu = Parent::base;
			int pi = 0, pe = NofOfs;
			for (; pi<pe && Parent::ofs[pi]; accu += getEndOfs(Parent::ofs[pi]),++pi){}
			if (pi == pe) return false;
			Parent::ofs[ pi] = packedRange( accu, range);
			if (!Parent::ofs[ pi]) return false;
		}
		return true;
	}

	PositionType end() const
	{
		PositionType rt=Parent::base;
		for (int ii=0; ii<=NofOfs && Parent::ofs[ii]; rt+=getEndOfs(Parent::ofs[ii]),++ii){}
		return rt;
	}

	strus::IndexRange skip( PositionType pos) const
	{
		PositionType rtend = Parent::base;
		int ii = 0;
		for (; ii<=NofOfs && rtend < pos; rtend+=getEndOfs(Parent::ofs[ii]),++ii){}
		if (rtend > pos) return strus::IndexRange( rtend - getSize( Parent::ofs[ii]), rtend);
		return strus::IndexRange();
	}
};

typedef StructBlockMemberPacked<1> StructBlockMemberPackedByte;
typedef StructBlockMemberPacked<2> StructBlockMemberPackedShort;

}//namespace
#endif


