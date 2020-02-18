/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_INDEX_BLOCK_DOC_INDEX_NODE_HPP_INCLUDED
#define _STRUS_INDEX_BLOCK_DOC_INDEX_NODE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <utility>

namespace strus {

struct DocIndexNode
{
	enum {Size=7};
	DocIndexNode();
	DocIndexNode( const DocIndexNode& o);

	bool addDocument( const Index& docno_, unsigned short ref_);
	Index skipDoc( const Index& docno_, unsigned short& cursor_docidx) const;
	Index firstDoc( unsigned short& cursor_docidx) const	{cursor_docidx=0; return base;}
	Index firstDoc() const					{return base;}
	Index nextDoc( unsigned short& cursor_docidx) const;
	Index lastDoc() const;
	std::size_t nofElements() const;

	Index base;
	unsigned short ofs[ Size-1];
	unsigned short ref[ Size];
};

struct DocIndexNodeCursor
{
	unsigned short nodeidx;
	unsigned short docidx;

	DocIndexNodeCursor()
		:nodeidx(0),docidx(0){}
	DocIndexNodeCursor( const DocIndexNodeCursor& o)
		:nodeidx(o.nodeidx),docidx(o.docidx){}
	void reset()
	{
		nodeidx = 0;
		docidx = 0;
	}
};

struct DocIndexNodeArray
{
	const DocIndexNode* ar;
	int size;

	DocIndexNodeArray()
		:ar(0),size(0){}
	DocIndexNodeArray( const DocIndexNode* ar_, int size_)
		:ar(ar_),size(size_){}
	DocIndexNodeArray( const DocIndexNodeArray& o)
		:ar(o.ar),size(o.size){}
	void init( const DocIndexNode* ar_, int size_)
		{ar=ar_;size=size_;}

	Index docno_at( const DocIndexNodeCursor& cursor) const;
	int ref_at( const DocIndexNodeCursor& cursor) const
	{
		return ar[ cursor.nodeidx].ref[ cursor.docidx];
	}
	const DocIndexNode& operator[]( const DocIndexNodeCursor& cursor) const
	{
		return ar[ cursor.nodeidx];
	}
	Index firstDoc( DocIndexNodeCursor& cursor) const;
	Index nextDoc( DocIndexNodeCursor& cursor) const;
	Index skipDoc( const Index& docno, DocIndexNodeCursor& cursor) const;
	Index lastDoc() const;
};

} //namespace
#endif

