/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_MEMBER_REPEAT_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_MEMBER_REPEAT_HPP_INCLUDED
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

namespace strus {

struct StructBlockMemberRepeat
{
	typedef unsigned short PositionType;
	typedef unsigned char OffsetType;
	enum {NofOfs=3*sizeof(PositionType)};

	PositionType base;
	OffsetType ofs;
	OffsetType size;

	static PositionType positionCast( strus::Index pos)
	{
		return (pos <= (strus::Index)std::numeric_limits<PositionType>::max()) ? (PositionType)pos : 0;
	}
	static OffsetType offsetCast( strus::Index ofs)
	{
		return (ofs <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)pos : 0;
	}
	static OffsetType sizeCast( strus::Index ofs)
	{
		return (ofs <= (strus::Index)std::numeric_limits<OffsetType>::max()) ? (OffsetType)pos : 0;
	}

	StructBlockMemberRepeat()
		:base(0),ofs(0),size(0){}
	explicit StructBlockMemberRepeat( PositionType base_, OffsetType ofs_, OffsetType size_)
		:base(base_),ofs(ofs_),size(size_){}
	StructBlockMemberRepeat( const StructBlockMemberEnum& o)
		:base(o.base),ofs(o.ofs),size(o.size){}

	bool append( strus::Index& end, const strus::IndexRange& range)
	{
		if (!base)
		{
			base = positionCast( range.start());
			size = sizeCast( range.size() - range.start());
			ofs = offsetCast( end - range.end());
			if (!base || !size || !ofs)
			{
				base = 0; size = 0; ofs = 0;
				return false;
			}
			return true;
		}
		else
		{
			strus::Index membsize = range.end() - range.start();
			strus::Index membofs = range.end() - end;

			if (sizeCast( membsize) == size && offsetCast( membofs) == ofs)
			{
				end += membofs;
				return true;
			}
			return false;
		}
	}

	strus::Index append( const std::vector<strus::IndexRange>& rangear)
	{
		if (rangear.size() < 3) return false;
		strus::Index rt = rangear[1].end() - rangear[0].end();
		std::vector<strus::IndexRange>::const_iterator ri = rangear.begin(), re = rangear.end();
		for (; ri != re && append( end, *ri); ++ri){}
		return ri == re ? rt : 0;
	}

	strus::IndexRange skip( PositionType pos)
	{
		Index start = base;
		Index rpos = (pos < base) ? 0 : (pos - base);
		Index rmod = rpos - rpos % ofs + base;
		if (rmod + size > pos)
		{
			return strus::IndexRange( rmod, rmod + size);
		}
		else
		{
			return strus::IndexRange( rmod + ofs, rmod + ofs + size);
		}
	}
};

}//namespace
#endif


