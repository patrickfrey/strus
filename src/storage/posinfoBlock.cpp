/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "posinfoBlock.hpp"
#include "memBlock.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <iostream>

using namespace strus;

enum {EndPosinfoMarker=(char)0xFE};

Index PosinfoBlock::docno_at( const Cursor& cursor) const
{
	const DocIndexNode& nd = m_docindexptr[ cursor.nodeidx];
	if (cursor.docidx)
	{
		return nd.base + nd.ofs[ cursor.docidx-1];
	}
	else
	{
		return nd.base;
	}
}

const PosinfoBlock::PositionType* PosinfoBlock::posinfo_at( const Cursor& cursor) const
{
	const DocIndexNode& nd = m_docindexptr[ cursor.nodeidx];
	return m_posinfoptr + nd.posrefIdx[ cursor.docidx];
}

std::vector<Index> PosinfoBlock::positions_at( const Cursor& cursor) const
{
	const PositionType* pi = posinfo_at( cursor);
	const PositionType* pe = pi + *pi + 1;
	std::vector<Index> rt;
	for (++pi; pi != pe; ++pi)
	{
		rt.push_back( *pi);
	}
	return rt;
}

unsigned int PosinfoBlock::frequency_at( const Cursor& cursor) const
{
	const DocIndexNode& nd = m_docindexptr[ cursor.nodeidx];
	return m_posinfoptr[ nd.posrefIdx[ cursor.docidx]];
}

Index PosinfoBlock::firstDoc( Cursor& cursor) const
{
	cursor.reset();
	return m_nofDocIndexNodes?m_docindexptr[ 0].base:0;
}

Index PosinfoBlock::nextDoc( Cursor& cursor) const
{
	if (++cursor.docidx >= DocIndexNode::Size)
	{
		cursor.docidx = 0;
		++cursor.nodeidx;
	}
	for (;;)
	{
		if (cursor.nodeidx >= m_nofDocIndexNodes)
		{
			cursor.reset();
			return 0;
		}
		if (cursor.docidx == 0)
		{
			return m_docindexptr[ cursor.nodeidx].base;
		}
		if (m_docindexptr[ cursor.nodeidx].ofs[ cursor.docidx-1])
		{
			return m_docindexptr[ cursor.nodeidx].base
				+ m_docindexptr[ cursor.nodeidx].ofs[ cursor.docidx-1];
		}
		else
		{
			cursor.docidx = 0;
			++cursor.nodeidx;
		}
	}
}

Index PosinfoBlock::skipDoc( const Index& docno_, Cursor& cursor) const
{
	std::size_t di = cursor.nodeidx;
	if (m_docindexptr[ di].base >= docno_)
	{
		if (m_docindexptr[ di].base == docno_)
		{
			cursor.docidx = 0;
			return docno_;
		}
		if (!di)
		{
			return m_docindexptr[ 0].base;
		}
		di = 0;
	}
	for (;di < m_nofDocIndexNodes && m_docindexptr[ di].base <= docno_; ++di){}
	if (!di)
	{
		if (m_nofDocIndexNodes)
		{
			cursor.reset();
			return m_docindexptr[ 0].base;
		}
		else
		{
			return 0;
		}
	}
	unsigned short cursor_docidx;
	Index rt = m_docindexptr[ di-1].skipDoc( docno_, cursor_docidx);
	if (rt)
	{
		cursor.nodeidx = (unsigned short)(di-1);
		cursor.docidx = cursor_docidx;
		return rt;
	}
	else
	{
		if (di < m_nofDocIndexNodes)
		{
			cursor.nodeidx = (unsigned short)di;
			cursor.docidx = 0;
			return m_docindexptr[ di].base;
		}
		else
		{
			return 0;
		}
	}
}

Index PosinfoBlock::PositionScanner::skip( const Index& pos)
{
	if ((Index)m_ar[ m_itr] >= pos)
	{
		while (m_itr > 0 && (Index)m_ar[ m_itr] > pos)
		{
			m_itr >>= 1;
		}
		if (m_ar[ m_itr] >= pos)
		{
			return m_ar[ m_itr];
		}
	}
	if (pos > (Index)std::numeric_limits<PositionType>::max()) return 0;

	unsigned int fibres = 0, fib1 = 1, fib2 = 1, ii = m_itr+1, nn = m_size;
	while (ii < nn && (Index)m_ar[ ii] < pos)
	{
		fibres = fib1 + fib2;
		ii += fibres;
		fib1 = fib2;
		fib2 = fibres;
	}
	for (ii -= fibres; ii < nn && (Index)m_ar[ ii] < pos; ++ii){}
	return ii < nn ? m_ar[ m_itr = (unsigned short)ii]:0;
}

void PosinfoBlock::initDocIndexNodeFrame()
{
	if (size() < sizeof(unsigned int))
	{
		m_nofDocIndexNodes = 0;
		m_docindexptr = 0;
		m_posinfoptr = 0;
	}
	else
	{
		m_nofDocIndexNodes = *(const unsigned int*)ptr();
		m_docindexptr = (const DocIndexNode*)( (const unsigned int*)ptr()+1);
		m_posinfoptr = (const PositionType*)(const void*)(m_docindexptr + m_nofDocIndexNodes);
	}
}

PosinfoBlock::DocIndexNode::DocIndexNode()
{
	std::memset( this, 0, sizeof(*this));
}

PosinfoBlock::DocIndexNode::DocIndexNode( const DocIndexNode& o)
{
	std::memcpy( this, &o, sizeof(*this));
}

std::size_t PosinfoBlock::DocIndexNode::nofElements() const
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

bool PosinfoBlock::DocIndexNode::addDocument( const Index& docno_, unsigned short posrefIdx_)
{
	if (!base)
	{
		base = docno_;
		posrefIdx[ 0] = posrefIdx_;
	}
	else
	{
		if (base >= docno_)
		{
			throw strus::runtime_error( _TXT( "documents not added in ascending order into posinfo block"));
		}
		if (base + std::numeric_limits<unsigned short>::max() <= docno_)
		{
			return false;
		}
		std::size_t ii = 0;
		for (; ii<(Size-1) && ofs[ii]; ++ii){}
		if (ii == (Size-1)) return false;
		ofs[ ii] = docno_ - base;
		posrefIdx[ ii+1] = posrefIdx_;
	}
	return true;
}

Index PosinfoBlock::DocIndexNode::skipDoc( const Index& docno_, unsigned short& cursor_docidx) const
{
	if (docno_ <= base)
	{
		if (docno_ == base)
		{
			cursor_docidx = 0;
			return base;
		}
		return 0;
	}
	if (docno_ - base > std::numeric_limits<unsigned short>::max()) return false;
	unsigned short ofs_ = (unsigned short)(docno_ - base);
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


PosinfoBlockBuilder::PosinfoBlockBuilder( const PosinfoBlock& o)
	:m_id(o.id())
{
	PosinfoBlock::Cursor idx;
	Index docno;
	if (!(docno=o.firstDoc( idx))) return;
	do
	{
		append( m_lastDoc = docno, o.posinfo_at( idx));
	}
	while (!!(docno=o.nextDoc( idx)));
}

void PosinfoBlockBuilder::append( const Index& docno, const PositionType* posar)
{
	if (m_id && m_id < docno) throw strus::runtime_error( _TXT( "assigned illegal id to block"));

	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
		{
			throw strus::logic_error( _TXT( "corrupt structure in posinfo block builder"));
		}
	}
	PositionType ii=0, nn=posar[0];
	for (; ii<=nn; ++ii)
	{
		m_posinfoArray.push_back( posar[ii]);
	}
	m_lastDoc = docno;
}

bool PosinfoBlockBuilder::fitsInto( std::size_t nofpos) const
{
	return m_posinfoArray.size() + nofpos <= std::numeric_limits<PositionType>::max();
}

PosinfoBlock PosinfoBlockBuilder::createBlock() const
{
	if (empty()) throw strus::runtime_error( _TXT( "tried to create empty posinfo block"));

	std::size_t blksize =
		sizeof( unsigned int)
		+ m_posinfoArray.size() * sizeof( m_posinfoArray[0])
		+ m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);

	MemBlock blkmem( blksize);
	unsigned int nofDocIndexNodes = *(unsigned int*)blkmem.ptr() = docIndexNodeArray().size();
	PosinfoBlock::DocIndexNode* docindexptr = (PosinfoBlock::DocIndexNode*)( (const unsigned int*)blkmem.ptr()+1);
	PositionType* posinfoptr = (PositionType*)(const void*)(docindexptr + nofDocIndexNodes);
	
	std::vector<DocIndexNode>::const_iterator
		di = m_docIndexNodeArray.begin(),
		de = m_docIndexNodeArray.end();
	for (std::size_t didx=0; di != de; ++di,++didx)
	{
		docindexptr[ didx] = *di;
	}
	std::vector<PositionType>::const_iterator
		pi = m_posinfoArray.begin(),
		pe = m_posinfoArray.end();
	for (std::size_t pidx=0; pi != pe; ++pi,++pidx)
	{
		posinfoptr[ pidx] = *pi;
	}
	return PosinfoBlock( m_id?m_id:m_lastDoc, blkmem.ptr(), blksize, true);
}

void PosinfoBlockBuilder::clear()
{
	m_docIndexNodeArray.clear();
	m_posinfoArray.clear();
	m_lastDoc = 0;
	m_id = 0;
}

void PosinfoBlockBuilder::setId( const Index& id_)
{
	if (id_ && id_ < m_lastDoc) throw strus::runtime_error( _TXT( "assigning illegal id to block"));
	m_id = id_;
}




