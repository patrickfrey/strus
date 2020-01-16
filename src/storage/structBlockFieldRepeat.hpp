/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_FIELD_REPEAT_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_FIELD_REPEAT_HPP_INCLUDED
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <vector>

namespace strus {

struct StructBlockFieldRepeat
{
	typedef unsigned short PositionType;
	typedef unsigned char OffsetType;

	PositionType base;
	OffsetType ofs;
	OffsetType size;

	bool check() const
	{
		return (size && ofs > size && base);
	}
	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static OffsetType offsetCast( strus::Index ofs)
	{
		return (ofs <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)ofs : 0;
	}
	static OffsetType sizeCast( strus::Index sz)
	{
		return (sz <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)sz : 0;
	}

	StructBlockFieldRepeat()
		:base(0),ofs(0),size(0){}
	explicit StructBlockFieldRepeat( PositionType base_, OffsetType ofs_, OffsetType size_)
		:base(base_),ofs(ofs_),size(size_){}
	StructBlockFieldRepeat( const StructBlockFieldRepeat& o)
		:base(o.base),ofs(o.ofs),size(o.size){}

	bool appendFirstPair( strus::Index& nextStart, const strus::IndexRange& range1, const strus::IndexRange& range2)
	{
		if (range1.len() != range2.len()) return false;

		base = positionCast( range1.start());
		size = sizeCast( range1.len());
		ofs  = offsetCast( range2.start() - range1.start());
		if (!ofs ||!size) return false;
		nextStart = range2.start() + ofs;
		return true;
	}

	bool appendNext( strus::Index& nextStart, const strus::IndexRange& range)
	{
		if (nextStart != range.start()) return false;
		if (sizeCast( range.len()) != size) return false;

		nextStart += ofs;
		return true;
	}

	int nofMembers( PositionType end) const
	{
		return (end - base + (ofs - size)) / ofs;
	}

	std::pair<strus::IndexRange,int> skip( PositionType pos, PositionType end) const
	{
		strus::Index rpos = (pos < base) ? 0 : (pos - base);
		strus::Index rmod = rpos % ofs;
		strus::Index rcnt = rpos / ofs;
		strus::Index rstart = rpos - rmod + base;

		if (rstart + size > pos)
		{
			return std::pair<strus::IndexRange,int>( strus::IndexRange( rstart, rstart + size), rcnt);
		}
		else if (rstart + ofs + size <= end)
		{
			return std::pair<strus::IndexRange,int>( strus::IndexRange( rstart + ofs, rstart + ofs + size), rcnt+1);
		}
		else
		{
			return std::pair<strus::IndexRange,int>( strus::IndexRange(), -1);
		}
	}
};

}//namespace
#endif


