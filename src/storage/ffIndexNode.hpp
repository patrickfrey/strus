/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_INDEX_BLOCK_FF_INDEX_NODE_HPP_INCLUDED
#define _STRUS_INDEX_BLOCK_FF_INDEX_NODE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/base/uintCompaction.hpp"
#include <vector>
#include <utility>
#include <algorithm>
#include <limits>

namespace strus {

struct FfIndexNode
{
	FfIndexNode()
		:base(0),ofs(0){ff[0]=0;ff[1]=0;}
	FfIndexNode( const FfIndexNode& o)
		:base(o.base),ofs(o.ofs){ff[0]=o.ff[0];ff[1]=o.ff[1];}

	bool setDocumentFf( const strus::Index& docno_, unsigned int ff_)
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
			if (docno_ - base > (Index)std::numeric_limits<unsigned short>::max()) return false;

			ofs = (unsigned short)(unsigned int)(int)(docno_ - base);
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

	strus::Index skipDoc( const strus::Index& docno_, strus::Index& ff_) const
	{
		if (docno_ <= base) {ff_ = strus::uintFromCompaction( ff[0]); return base;}
		if (docno_ > base + ofs) {ff_ = 0; return 0;}
		ff_ = strus::uintFromCompaction( ff[1]); return base + ofs;
	}

	std::size_t nofElements() const
	{
		return base ? (ofs ? 2 : 1) : 0;
	}

	strus::Index firstDoc() const			{return base;}
	strus::Index lastDoc() const			{return base+ofs;}
	bool hasSecond() const				{return ofs;}

	strus::Index ff_at( int elemidx) const		{return elemidx ? strus::uintFromCompaction( ff[1]) : strus::uintFromCompaction( ff[0]);}
	strus::Index docno_at( int elemidx) const	{return elemidx ? base + ofs : base;}

private:
	strus::Index base;
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

	strus::Index docno_at( const FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			return 0;
		}
		else
		{
			return ar[ cursor.nodeidx].docno_at( cursor.elemidx);
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
			return (unsigned int)ar[ cursor.nodeidx].ff_at( cursor.elemidx);
		}
	}

	strus::Index firstDoc( FfIndexNodeCursor& cursor) const
	{
		cursor.reset();
		return docno_at( cursor);
	}
	strus::Index nextDoc( FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			return 0;
		}
		else if (cursor.elemidx == 0 && ar[ cursor.nodeidx].hasSecond())
		{
			return ar[ cursor.nodeidx].docno_at( cursor.elemidx = 1);
		}
		else
		{
			++cursor.nodeidx;
			cursor.elemidx = 0;
			
			return cursor.nodeidx == size ? 0 : ar[ cursor.nodeidx].firstDoc();
		}
	}
	strus::Index skipDoc( const strus::Index& docno, FfIndexNodeCursor& cursor) const
	{
		if (cursor.nodeidx == size)
		{
			if (size == 0) return 0;
			cursor.reset();
		}
		else if (ar[ cursor.nodeidx].firstDoc() > docno)
		{
			cursor.reset();
		}
		else if (ar[ cursor.nodeidx].lastDoc() >= docno)
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

	strus::Index lastDoc() const
	{
		if (size == 0) return 0;
		return ar[ size-1].lastDoc();
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
	strus::Index selectCursorDocno( const strus::Index& docno, FfIndexNodeCursor& cursor) const
	{
		if (ar[ cursor.nodeidx].firstDoc() >= docno)
		{
			cursor.elemidx = 0;
			return ar[ cursor.nodeidx].firstDoc();
		}
		else
		{
			cursor.elemidx = 1;
			return ar[ cursor.nodeidx].lastDoc();
		}
	}
	int linSearchIndex( const strus::Index& docno, int eidx) const
	{
		for (; eidx < size && ar[ eidx].lastDoc() < docno; ++eidx){}
		return eidx;
	}
	int fibSearchIndex( const strus::Index& docno, int eidx) const
	{
		// Fibonacci search: f1,f2 = f1+f2,f1 => f2 += f1, swap( f1, f2)
		int f1 = 1, f2 = 1;
		int startidx = eidx;
		for(;;)
		{
			eidx = startidx + f1;
			if (eidx < size)
			{
				if (ar[ eidx].firstDoc() > docno)
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



