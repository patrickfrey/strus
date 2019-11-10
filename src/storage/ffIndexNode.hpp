/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_INDEX_BLOCK_FF_INDEX_NODE_HPP_INCLUDED
#define _STRUS_INDEX_BLOCK_FF_INDEX_NODE_HPP_INCLUDED
#include "strus/index.hpp"
#include "uintCompaction.hpp"
#include <utility>
#include <algorithm>

namespace strus {

struct FfIndexNode
{
	FfIndexNode()
		:base(0),ofs(0){ff[0]=0;ff[1]=0;}
	FfIndexNode( const FfIndexNode& o)
		:base(o.base),ofs(o.ofs){ff[0]=o.ff[0];ff[1]=o.ff[1];}

	bool setDocumentFf( const Index& docno_, unsigned int ff_)
	{
		if (ofs)
		{
			if (base + ofs == docno_)
			{
				ff[1] = strus::compactUint( ff_);
				return true;
			}
			return false;
		}
		else if (docno_ < base)
		{
			return false;
		}
		else if (base && base != docno_)
		{
			if (base + ofs < docno_) return false;

			ofs = (unsigned int)(docno_ - base);
			ff[1] = strus::compactUint( ff_);
			return true;
		}
		else
		{
			base = docno_;
			ff[0] = strus::compactUint( ff_);
			return true;
		}
	}

	Index skipDoc( const Index& docno_, strus::Index& ff_) const
	{
		if (docno_ <= base) {ff_ = ff[0]; return base;}
		if (docno_ > base + ofs) {ff_ = 0; return 0;}
		ff_ = ff[1]; return base + ofs;
	}

	std::size_t nofElements() const
	{
		return base ? (ofs ? 2 : 1) : 0;
	}

	Index firstDoc() const		{return base;}
	Index lastDoc() const		{return base+ofs;}

	Index base;
	unsigned short ofs;
	uint8_t ff[ 2];
};

struct FfIndexNodeCursor
{
	int nodeidx;
	int elemidx;

	FfIndexNodeCursor()
		:nodeidx(0),elemidx(0){}
	FfIndexNodeCursor( const FfIndexNodeCursor& o)
		:nodeidx(o.nodeidx),elemidx(o.elemidx){}
	void reset()
	{
		nodeidx = 0;
		elemidx = 0;
	}
};

struct FfIndexNodeArray
{
	const FfIndexNode* ar;
	int size;

	FfIndexNodeArray()
		:ar(0),size(0){}
	FfIndexNodeArray( const FfIndexNodeArray& o)
		:ar(o.ar),size(o.size){}
	void init( const FfIndexNode* ar_, int size_)
		{ar=ar_;size=size_;}

	Index docno_at( const FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			return 0;
		}
		else
		{
			Index rt = ar[ cursor.nodeidx].base;
			if (cursor.elemidx)
			{
				rt += ar[ cursor.nodeidx].ofs;
			}
			return rt;
		}
	}
	int ff_at( const FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			return 0;
		}
		else
		{
			return (unsigned int)ar[ cursor.nodeidx].ff[ cursor.elemidx];
		}
	}

	Index firstDoc( FfIndexNodeCursor& cursor) const
	{
		cursor.reset();
		return docno_at( cursor);
	}
	Index nextDoc( FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			return 0;
		}
		else if (cursor.elemidx == 0 && ar[ cursor.nodeidx].ofs)
		{
			cursor.elemidx = 1;
			return ar[ cursor.nodeidx].base + ar[ cursor.nodeidx].ofs;
		}
		else
		{
			++cursor.nodeidx;
			cursor.elemidx = 0;
			
			return cursor.nodeidx == size ? 0 : ar[ cursor.nodeidx].base;
		}
	}
	Index skipDoc( const Index& docno, FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			if (size == 0) return 0;
			cursor.reset();
		}
		else if (ar[ cursor.nodeidx].base > docno)
		{
			cursor.reset();
		}
		else if (ar[ cursor.nodeidx].base + ar[ cursor.nodeidx].ofs >= docno)
		{
			return selectCursorDocno( docno, cursor);
		}
		if (size == (cursor.nodeidx = fibSearchIndex( docno, cursor.nodeidx)))
		{
			cursor.elemidx = 0;
			return 0;
		}
		return selectCursorDocno( docno, cursor);
	}

	Index lastDoc() const
	{
		if (size == 0) return 0;
		return ar[ size-1].base + ar[ size-1].ofs;
	}

	std::vector<FfIndexNode> tovector() const
	{
		std::vector<FfIndexNode> rt;
		int ai = 0, ae = size;
		for (; ai != ae; ++ai)
		{
			rt.push_back( ar[ ai]);
		}
		return rt;
	}

private:
	Index selectCursorDocno( const Index& docno, FfIndexNodeCursor& cursor) const
	{
		if (ar[ cursor.nodeidx].base >= docno)
		{
			cursor.elemidx = 0;
			return ar[ cursor.nodeidx].base;
		}
		else
		{
			cursor.elemidx = 1;
			return ar[ cursor.nodeidx].base + ar[ cursor.nodeidx].ofs;
		}
	}
	int linSearchIndex( const Index& docno, int eidx) const
	{
		for (; eidx < size && ar[ eidx].base + ar[ eidx-1].ofs < docno; ++eidx){}
		return eidx;
	}
	int fibSearchIndex( const Index& docno, int eidx) const
	{
		// Fibonacci search: f1,f2 = f1+f2,f1 => f2 += f1, swap( f1, f2)
		int f1 = 1, f2 = 1;
		int startidx = eidx;
		for(;;)
		{
			eidx = startidx + f1;
			if (eidx < size)
			{
				if (ar[ eidx].base > docno)
				{
					return linSearchIndex( docno, startidx);
				}
			}
			else
			{
				return linSearchIndex( docno, startidx);
			}
			f2 += f1;
			std::swap( f2, f1);
		}
	}
};

}//namespace
#endif



