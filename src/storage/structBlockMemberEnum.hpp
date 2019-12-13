/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_MEMBER_ENUM_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_MEMBER_ENUM_HPP_INCLUDED
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstring>

namespace strus {

struct StructBlockMemberEnum
{
	typedef unsigned short PositionType;
	typedef unsigned char OffsetType;
	enum {NofOfs=3*sizeof(PositionType)};

	PositionType base;
	OffsetType ofs[ NofOfs];

	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static OffsetType offsetCast( strus::Index ofs)
	{
		return (ofs <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)pos : 0;
	}

	explicit StructBlockMemberEnum( PositionType base_=0);
	StructBlockMemberEnum( const StructBlockMemberEnum& o);

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
			if (pos < accu) throw std::runtime_error(_TXT("structure elements not added in ascending order"));
			if (pos - accu > (PositionType)std::numeric_limits<OffsetType>::max()) return false;
			ofs[ pi] = pos - accu;
		}
		return true;
	}

	PositionType last() const
	{
		PositionType rt=base;
		for (int ii=0; ii<=NofOfs; rt+=ofs[ii],++ii){}
		return rt;
	}

	strus::IndexRange skip( PositionType pos)
	{
		PositionType pi=base;
		PositionType start=base;
		int ii=0;
		for (; ii<=NofOfs && pi < pos; pi+=ofs[ii],++ii)
		{
			if (1!=ofs[ii]) {start=pi+ofs[ii];}
		}
		if (pi >= pos)
		{
			PositionType end = pi+1;
			for (; ii<=NofOfs && 1==ofs[ii]; ++end,++ii){}
			return strus::IndexRange( start, end);
		}
		return strus::IndexRange();
	}

	strus::Index expandEnd( strus::Index end)
	{
		if (end == base)
		{
			strus::Index rt = base+1;
			for (int ii=0; ii<=NofOfs && 1==ofs[ii]; ++rt,++ii){}
			return rt;
		}
		return end;
	}

	strus::Index expandStart( strus::Index start)
	{
		for (int ii=NofOfs-1; ii>=0 && 0==ofs[ii]; --ii){}
		for (int ii=NofOfs-1; ii>=0 && 1==ofs[ii]; --ii,--start){}
		return start;
	}
};

}//namespace
#endif


