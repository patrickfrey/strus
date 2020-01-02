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

	bool append( strus::Index& enditr, const strus::IndexRange& range)
	{
		if (!base)
		{
			base = positionCast( range.start());
			size = sizeCast( range.end() - range.start());
			ofs = offsetCast( enditr - range.start());
			if (!base || !size || !ofs)
			{
				base = 0; size = 0; ofs = 0;
				return false;
			}
			enditr += ofs;
			return true;
		}
		else
		{
			strus::Index membsize = range.end() - range.start();
			strus::Index membofs = enditr - range.start();

			if (sizeCast( membsize) == size && offsetCast( membofs) == ofs)
			{
				enditr += membofs;
				return true;
			}
			return false;
		}
	}

	strus::Index append( const std::vector<strus::IndexRange>& rangear)
	{
		if (rangear.size() < 3) return false;
		strus::Index rt = rangear[0].start() + (rangear[1].end() - rangear[0].end());
		std::vector<strus::IndexRange>::const_iterator ri = rangear.begin(), re = rangear.end();
		for (; ri != re && append( rt, *ri); ++ri){}
		return ri == re ? rt : 0;
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


