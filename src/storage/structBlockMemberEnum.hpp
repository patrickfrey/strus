/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_MEMBER_ENUM_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_MEMBER_ENUM_HPP_INCLUDED
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <cstring>

namespace strus {

struct StructBlockMemberEnum
{
	typedef unsigned short PositionType;
	enum {NofOfs=3*sizeof(PositionType)};

	PositionType base;
	unsigned char ofs[ NofOfs];

	explicit StructBlockMemberEnum( PositionType base_=0);
	StructBlockMemberEnum( const StructBlockMemberEnum& o);

	bool full() const {return !!ofs[NofOfs-1];}
	bool append( PositionType pos);

	PositionType last() const {PositionType rt=base; for (int i=0; i<=NofOfs; rt += ofs[i]){} return rt;}

	struct SearchCompare
	{
		SearchCompare(){}
		bool operator()( const StructBlockMemberEnum& aa, const PositionType& bb) const
		{
			return aa.base <= bb;
		}
	};

	struct Iterator
	{
		typedef SkipScanArray<StructBlockMemberEnum,strus::Index,StructBlockMemberEnum::SearchCompare> Ar;

		struct Element
		{
			int aidx;
			int eidx;
			strus::Index cur;

			Element() :aidx(0),eidx(-1),cur(0){}
			Element( const Element& o) :aidx(o.aidx),eidx(o.eidx),cur(o.cur){}
			Element& operator=( const Element& o) {aidx=o.aidx; eidx=o.eidx; cur=o.cur; return *this;}

			strus::IndexRange skipPos( const Ar& ar, strus::Index pos);

		private:
			bool skipEnd( const Ar& ar);
			bool skipStart( const Ar& ar);
			strus::IndexRange range( const Ar& ar) const;
			strus::IndexRange seekPos( const Ar& ar, strus::Index pos, int startidx, int endidx);
		};

		Ar ar;
		Element element;
		strus::IndexRange cur;

		Iterator()
			:ar(0,0),element(),cur(){}
		Iterator( const StructBlockMemberEnum* ar_, std::size_t arsize_)
			:ar(ar_,arsize_),element(),cur(){}
		Iterator( const Iterator& o)
			:ar(o.ar),element(o.element),cur(o.cur){}
		Iterator& operator=( const Iterator& o)
			{ar=o.ar; element=o.element; cur=o.cur; return *this;}

		IndexRange next();
		IndexRange current()	{return cur;}

		strus::IndexRange skip( strus::Index pos);
	};
};

}//namespace
#endif


