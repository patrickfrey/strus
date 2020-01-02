/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_FIELD_ENUM_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_FIELD_ENUM_HPP_INCLUDED
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstring>

namespace strus {

struct StructBlockFieldEnum
{
	typedef unsigned short PositionType;
	typedef unsigned char OffsetType;
	enum {NofOfs=3*sizeof(PositionType)};

	PositionType base;
	OffsetType ofs[ NofOfs];

	bool check() const
	{
		int pi=0, pe=NofOfs;
		for (; pi!=pe && ofs[pi]; ++pi){}
		for (; pi!=pe && !ofs[pi]; ++pi){}
		return !!base && pi == pe;
	}

	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static OffsetType offsetCast( strus::Index ofs)
	{
		return (ofs <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)ofs : 0;
	}

	explicit StructBlockFieldEnum( PositionType base_=0)
		:base(base_)
	{
		std::memset( ofs, 0, sizeof(ofs));
	}

	StructBlockFieldEnum( const StructBlockFieldEnum& o)
		:base(o.base)
	{
		std::memcpy( ofs, o.ofs, sizeof(ofs));
	}

	bool full() const {return !!ofs[NofOfs-1];}

	bool append( PositionType pos)
	{
		if (base == 0)
		{
			base = pos;
		}
		else
		{
			PositionType accu = base;
			int pi = 0, pe = NofOfs;
			for (; pi<pe && ofs[pi]; accu+=ofs[pi],++pi){}
			if (pi == pe) return false;
			if (pos <= accu) throw std::runtime_error(_TXT("structure elements not added in strictly ascending order"));
			if (pos - accu > (PositionType)std::numeric_limits<OffsetType>::max()) return false;
			ofs[ pi] = pos - accu;
		}
		return true;
	}

	bool append( const strus::IndexRange& range)
	{
		if (range.start()+1 != range.end()) return false;
		return append( range.start());
	}

	PositionType last() const
	{
		PositionType rt=base;
		for (int ii=0; ii<=NofOfs && ofs[ii]; rt+=ofs[ii],++ii){}
		return rt;
	}

	bool pop_back()
	{
		int ii = NofOfs-1;
		for (; ii>=0 && 0==ofs[ii]; --ii){}
		if (ii < 0)
		{
			base = 0;
		}
		else
		{
			ofs[ii] = 0;
		}
		return true;
	}

	std::pair<strus::IndexRange,int> skip( PositionType pos) const
	{
		PositionType pi=base;
		int ii=0;
		for (; ii<NofOfs && pi < pos && ofs[ii]; pi+=ofs[ii],++ii){}
		if (pi >= pos)
		{
			return std::pair<strus::IndexRange,int>( strus::IndexRange( pi, pi+1), ii);
		}
		else
		{
			return std::pair<strus::IndexRange,int>( strus::IndexRange(), -1);
		}
	}

	int nofMembers() const
	{
		int rt = 0;
		if (base)
		{
			int ii=0;
			for (; ii<NofOfs && ofs[ii]; ++ii){}
			return ii+1;
		}
		return rt;
	}
};

}//namespace
#endif


