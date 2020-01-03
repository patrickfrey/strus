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
#include <iostream>

namespace strus {

typedef unsigned short PackedStructBlockLink;

struct StructBlockLink
{
	StructBlockLink()
		:head(0),structno(0),idx(0){}
	StructBlockLink( strus::Index structno_, bool head_, int idx_)
		:head(head_),structno(structno_),idx(idx_){}
	StructBlockLink( const StructBlockLink& o)
		:head(o.head),structno(o.structno),idx(o.idx){}
	StructBlockLink& operator=( const StructBlockLink& o)
		{head=o.head; structno=o.structno; idx=o.idx; return *this;}

	bool head;			//...bit 15
	strus::Index structno;		//...bits 11 12 13 14, bound to MaxNofStructNo=16
	int idx;			//...rest bits 0..10, bound to MaxNofStructIdx=2048

	PackedStructBlockLink value() const
	{
		return ((head?1:0) << 15) + ((structno & 0xF) << 11) + ((idx & 2047));
	}

	void setValue( PackedStructBlockLink vv)
	{
		head = ((vv & (1 << 15)) != 0);
		structno = (vv >> 11) & 0xF;
		idx = vv & 2047;
	}

	bool operator < (const StructBlockLink& o) const
	{
		return head == o.head
			? (structno == o.structno
				? (idx < o.idx)
				: (structno < o.structno))
			: (head == true);
	}
	bool valid() const
	{
		return structno && idx;
	}
	bool operator==( const StructBlockLink& o) const
	{
		return head == o.head && structno == o.structno && idx == o.idx;
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

	strus::Index structno;		//...bits 11 12 13 14, bound to MaxNofStructNo=16
	int idx;			//...rest bits 0..10, bound to MaxNofStructIdx=2048

	bool operator < (const StructBlockKey& o) const
	{
		return structno == o.structno
				? (idx < o.idx)
				: (structno < o.structno);
	}
	bool valid() const
	{
		return structno && idx;
	}
	bool operator==( const StructBlockKey& o) const
	{
		return structno == o.structno && idx == o.idx;
	}
};

}//namespace
#endif

