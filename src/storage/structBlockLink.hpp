/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_LINK_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_LINK_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/base/packed.h"
#include <iostream>

namespace strus {

PACKED_STRUCT( StructBlockLink)
{
	StructBlockLink()
		:head(true),structno(0),idx(0){}
	StructBlockLink( strus::Index structno_, bool head_, unsigned short idx_)
		:head(head_),structno(structno_),idx(idx_){}
	StructBlockLink( const StructBlockLink& o)
		:head(o.head),structno(o.structno),idx(o.idx){}
	StructBlockLink& operator=( const StructBlockLink& o)
		{head=o.head; structno=o.structno; idx=o.idx; return *this;}

	unsigned int head:1;		//...bit 15
	unsigned int structno:7;	//...bound to StructBlock::MaxNofStructNo=(1<<7)-1,
	unsigned int idx:16;		//...bound to StructBlock::MaxNofStructIdx=(1<<16)-1

	bool operator < (const StructBlockLink& o) const
	{
		return head == o.head
			? (structno == o.structno
				? (idx < o.idx)
				: (structno < o.structno))
			: ((int)head > (int)o.head);
	}
	bool valid() const
	{
		return structno && idx;
	}
	bool operator==( const StructBlockLink& o) const
	{
		return head == o.head && structno == o.structno && idx == o.idx;
	}
	StructureLink unpacked() const
	{
		return StructureLink( (strus::Index)structno, (bool)head, (int)idx);
	}
	void print( std::ostream& out) const
	{
		const char* dir = head ? "->" : "<-";
		out << "[" << structno << "," << idx << "] " << dir;
	}
};


struct StructBlockKey
{
	StructBlockKey()
		:structno(0),idx(0){}
	StructBlockKey( strus::Index structno_, int idx_)
		:structno(structno_),idx(idx_){}
	StructBlockKey( const StructBlockKey& o)
		:structno(o.structno),idx(o.idx){}
	StructBlockKey& operator=( const StructBlockKey& o)
		{structno=o.structno; idx=o.idx; return *this;}

	strus::Index structno;		//...bound to StructBlock::MaxNofStructNo=(1<<7)-1,
	int idx;			//...bound to StructBlock::MaxNofStructIdx=(1<<16)-1

	bool valid() const
	{
		return structno && idx;
	}
	bool operator < (const StructBlockKey& o) const
	{
		return structno == o.structno
				? (idx < o.idx)
				: (structno < o.structno);
	}
	bool operator==( const StructBlockKey& o) const
	{
		return structno == o.structno && idx == o.idx;
	}
};

}//namespace
#endif

