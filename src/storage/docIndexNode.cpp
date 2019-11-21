/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "docIndexNode.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

using namespace strus;

DocIndexNode::DocIndexNode()
{
	std::memset( this, 0, sizeof(*this));
}

DocIndexNode::DocIndexNode( const DocIndexNode& o)
{
	std::memcpy( this, &o, sizeof(*this));
}

std::size_t DocIndexNode::nofElements() const
{
	if (!base)
	{
		return 0;
	}
	else
	{
		std::size_t ii = 0;
		for (; ii<(Size-1) && ofs[ii]; ++ii){}
		return ii+1;
	}
}

bool DocIndexNode::addDocument( const Index& docno, unsigned short ref_)
{
	if (!base)
	{
		base = docno;
		ref[ 0] = ref_;
	}
	else
	{
		if (base >= docno)
		{
			throw strus::runtime_error( _TXT( "documents not added in ascending order into posinfo block"));
		}
		if (base + std::numeric_limits<unsigned short>::max() <= docno)
		{
			return false;
		}
		std::size_t ii = 0;
		for (; ii<(Size-1) && ofs[ii]; ++ii){}
		if (ii == (Size-1)) return false;
		ofs[ ii] = (docno - base);
		ref[ ii+1] = ref_;
	}
	return true;
}

Index DocIndexNode::nextDoc( unsigned short& cursor_docidx) const
{
	if (cursor_docidx < 0)
	{
		cursor_docidx = 0;
		return base;
	}
	else
	{
		int ii = cursor_docidx++;
		if (ii < (Size-1) && ofs[ ii])
		{
			return base + ofs[ ii];
		}
		else
		{
			--cursor_docidx;
			return 0;
		}
	}
}

Index DocIndexNode::lastDoc() const
{
	int ii = 0;
	for (;ii<(Size-1) && ofs[ii];++ii){}
	return ii ? (base+ofs[ii-1]):base;
}

Index DocIndexNode::skipDoc( const Index& docno, unsigned short& cursor_docidx) const
{
	if (docno <= base)
	{
		if (docno == base)
		{
			cursor_docidx = 0;
			return base;
		}
		return 0;
	}
	if (docno - base > std::numeric_limits<unsigned short>::max()) return 0;
	unsigned short ofs_ = (docno - base);
	std::size_t ii = 0;
	for (; ii<(Size-1) && ofs_ > ofs[ii]; ++ii){}
	if (ii == (Size-1))
	{
		return 0;
	}
	else
	{
		cursor_docidx = ii+1;
		return base + ofs[ ii];
	}
}

Index DocIndexNodeArray::docno_at( const DocIndexNodeCursor& cursor) const
{
	const DocIndexNode& nd = ar[ cursor.nodeidx];
	if (cursor.docidx)
	{
		return nd.base + nd.ofs[ cursor.docidx-1];
	}
	else
	{
		return nd.base;
	}
}

Index DocIndexNodeArray::firstDoc( DocIndexNodeCursor& cursor) const
{
	cursor.reset();
	return size?ar[ 0].base:0;
}

Index DocIndexNodeArray::lastDoc() const
{
	return size ? ar[ size-1].lastDoc() : 0;
}
	
Index DocIndexNodeArray::nextDoc( DocIndexNodeCursor& cursor) const
{
	if (++cursor.docidx >= DocIndexNode::Size)
	{
		cursor.docidx = 0;
		++cursor.nodeidx;
	}
	for (;;)
	{
		if (cursor.nodeidx >= size)
		{
			cursor.reset();
			return 0;
		}
		if (cursor.docidx == 0)
		{
			return ar[ cursor.nodeidx].base;
		}
		if (ar[ cursor.nodeidx].ofs[ cursor.docidx-1])
		{
			return ar[ cursor.nodeidx].base + ar[ cursor.nodeidx].ofs[ cursor.docidx-1];
		}
		else
		{
			cursor.docidx = 0;
			++cursor.nodeidx;
		}
	}
}

Index DocIndexNodeArray::skipDoc( const Index& docno, DocIndexNodeCursor& cursor) const
{
	if (!size) return 0;
	int di = cursor.nodeidx;
	if (ar[ di].base >= docno)
	{
		if (ar[ di].base == docno)
		{
			cursor.docidx = 0;
			return docno;
		}
		if (di == 0)
		{
			return ar[ 0].base;
		}
		di = 0;
	}
	for (;di < size && ar[ di].base <= docno; ++di){}
	if (di == 0)
	{
		cursor.reset();
		return ar[ 0].base;
	}
	unsigned short cursor_docidx;
	Index rt = ar[ di-1].skipDoc( docno, cursor_docidx);
	if (rt)
	{
		cursor.nodeidx = (unsigned short)(di-1);
		cursor.docidx = cursor_docidx;
		return rt;
	}
	else
	{
		if (di < size)
		{
			cursor.nodeidx = (unsigned short)di;
			cursor.docidx = 0;
			return ar[ di].base;
		}
		else
		{
			return 0;
		}
	}
}

