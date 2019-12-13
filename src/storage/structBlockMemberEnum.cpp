/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockMemberEnum.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

using namespace strus;

StructBlockMemberEnum::StructBlockMemberEnum( PositionType base_)
	:base(base_)
{
	std::memset( ofs, 0, sizeof(ofs));
}

StructBlockMemberEnum::StructureMemberEnum( const StructureMemberEnum& o)
	:base(o.base)
{
	std::memcpy( ofs, o.ofs, sizeof(ofs));
}

bool StructBlockMemberEnum::append( PositionType pos)
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
		if (pos - accu > std::numeric_limits<unsigned char>::max()) return false;
		ofs[ pi] = pos - accu;
	}
	return true;
}

bool StructBlockMemberEnum::Iterator::Element::skipEnd( const Ar& ar)
{
	if (aidx == ar.size()) return false;
	for (;;)
	{
		if (eidx < 0) eidx=0;
		for (; eidx<NofOfs && 1==ar[aidx].ofs[eidx]; ++cur,++eidx){}
		if (eidx==NofOfs || 0==ar[aidx].ofs[eidx])
		{
			++aidx;
			eidx = 0;
			if (aidx < ar.size() && ar[aidx].base == cur+1)
			{
				++cur;
				continue;
			}
		}
		break;
	}
	return true;
}

bool StructBlockMemberEnum::Iterator::Element::skipStart( const Ar& ar)
{
	if (aidx == ar.size()) return false;
	for (;;)
	{
		for (; eidx>=0 && 1==ar[aidx].ofs[eidx]; --cur,--eidx){}
		if (eidx == -1 && ar[aidx].base == cur)
		{
			if (aidx == 0) return cur;
			if (ar[aidx-1].last() == cur-1)
			{
				--cur;
				--aidx;
				eidx = NofOfs-1;
				for (; eidx>=0 && 0==ar[aidx].ofs[eidx]; --eidx) {}
				continue;
			}
		}
		break;
	}
	return cur;
}

strus::IndexRange StructBlockMemberEnum::Iterator::Element::range( const Ar& ar) const
{
	Element start = *this;
	start.skipStart( ar);
	Element end = *this;
	end.skipEnd( ar);
	return strus::IndexRange( start.cur, end.cur+1);
}

strus::IndexRange StructBlockMemberEnum::Iterator::Element::seekPos( const Ar& ar, strus::Index pos, int startidx, int endidx)
{
	if (startidx == endidx) return strus::IndexRange();

	aidx = ar.upperbound( pos, startidx, endidx, StructureMemberEnum::SearchCompare());
	if (aidx < 0)
	{
		eidx = -1;
		aidx = endidx-1;
		cur = (Index)ar[ aidx].base;
		for (++eidx; eidx<NofOfs && ar[aidx].ofs[eidx] && cur < pos; cur+=ar[aidx].ofs[eidx],++eidx){}
		--eidx;
		return (cur >= pos) ? range(ar) : strus::IndexRange();
	}
	else if (aidx == 0)
	{
		eidx = -1;
		aidx = startidx;
		cur = (Index)ar[ aidx].base;
		return range(ar);
	}
	else
	{
		eidx = -1;
		aidx += startidx;
		cur = (Index)ar[ --aidx].base;
		for (++eidx; eidx<NofOfs && ar[aidx].ofs[eidx] && cur < pos; cur+=ar[aidx].ofs[eidx],++eidx){}
		--eidx;
		if (cur >= pos) return range(ar);
		++aidx;
		eidx = -1;
		cur = (Index)ar[ aidx].base;
		return range(ar);
	}
	
}

strus::IndexRange StructBlockMemberEnum::Iterator::Element::skipPos( const Ar& ar, strus::Index pos)
{
	if (aidx >= ar.size())
	{
		if (aidx == 0) return strus::IndexRange();
		aidx = 0;
		eidx =-1;
		cur = (Index)ar[ aidx].base;
	}
	if (!cur)
	{
		eidx =-1;
		cur = (Index)ar[ aidx].base;
	}
	if (cur && pos > cur && pos <= ar[aidx].last())
	{
		for (++eidx; eidx<NofOfs && ar[aidx].ofs[eidx] && cur < pos; cur+=ar[aidx].ofs[eidx],++eidx){}
		return range(ar);
	}
	else
	{
		if ((Index)ar[ aidx].base >= pos)
		{
			if (aidx == 0 || (Index)ar[ aidx].base == pos)
			{
				eidx = -1;
				cur = (Index)ar[ aidx].base;
				return range(ar);
			}
			else if ((Index)ar[ aidx-1].base <= pos)
			{
				eidx = -1;
				cur = (Index)ar[ --aidx].base;
				for (++eidx; eidx<NofOfs && ar[aidx].ofs[eidx] && cur < pos; cur+=ar[aidx].ofs[eidx],++eidx){}
				--eidx;
				return range(ar);
			}
			else
			{
				return seekPos( ar, pos, 0, aidx);
			}
		}
		else if (ar[aidx].last() >= pos)
		{
			eidx = -1;
			cur = (Index)ar[ aidx].base;
			for (++eidx; eidx<NofOfs && ar[aidx].ofs[eidx] && cur < pos; cur+=ar[aidx].ofs[eidx],++eidx){}
			--eidx;
			return range(ar);
		}
		else
		{
			return seekPos( ar, pos, aidx, ar.size());
		}
	}
}

strus::IndexRange StructBlockMemberEnum::Iterator::next()
{
	return cur = element.skipPos( ar, cur.end());
}

strus::IndexRange StructBlockMemberEnum::Iterator::skip( strus::Index pos)
{
	if (cur.end() <= pos || cur.start() > pos)
	{
		cur = element.skipPos( ar, pos);
	}
	return cur;
}
