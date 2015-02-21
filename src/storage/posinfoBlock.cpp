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
#include <cstring>
#include <limits>
#include <iostream>
#include <boost/scoped_array.hpp>

using namespace strus;

enum {EndPosinfoMarker=(char)0xFE};

Index PosinfoBlock::docno_at( const Cursor& idx) const
{
	unsigned int ofs = idx & 0xF;
	const DocIndexNode& nd = m_docindexptr[ idx >> 4];
	if (ofs)
	{
		return nd.base + nd.ofs[ ofs-1];
	}
	else
	{
		return nd.base;
	}
}

std::vector<Index> PosinfoBlock::positions_at( const Cursor& idx) const
{
	unsigned int ofs = idx & 0xF;
	const DocIndexNode& nd = m_docindexptr[ idx >> 4];
	const PositionType* pi = m_posinfoptr + nd.posrefIdx[ ofs];
	const PositionType* pe = pi + *pi + 1;
	std::vector<Index> rt;
	for (++pi; pi != pe; ++pi)
	{
		rt.push_back( *pi);
	}
	return rt;
}

unsigned int PosinfoBlock::frequency_at( const Cursor& idx) const
{
	unsigned int ofs = idx & 0xF;
	const DocIndexNode& nd = m_docindexptr[ idx >> 4];
	const PositionType* pi = m_posinfoptr + nd.posrefIdx[ ofs];
	return *pi;
}

const PosinfoBlock::PositionType* PosinfoBlock::posinfo_at( const Cursor& idx) const
{
	unsigned int ofs = idx & 0xF;
	const DocIndexNode& nd = m_docindexptr[ idx >> 4];
	return m_posinfoptr + nd.posrefIdx[ ofs];
}

Index PosinfoBlock::firstDoc( Cursor& idx) const
{
	idx = 0;
	return m_nofDocIndexNodes?m_docindexptr[ 0].base:0;
}

Index PosinfoBlock::nextDoc( Cursor& idx) const
{
	unsigned int ndidx = idx >> 4;
	unsigned int ofs = (idx+1) & 0xF;
	if (ofs >= DocIndexNode::Size)
	{
		ofs = 0;
		++ndidx;
	}
	for (;;)
	{
		if (ndidx >= m_nofDocIndexNodes)
		{
			idx = 0;
			return 0;
		}
		if (ofs == 0)
		{
			idx = (ndidx << 4) + ofs;
			return m_docindexptr[ ndidx].base;
		}
		if (m_docindexptr[ ndidx].ofs[ ofs-1])
		{
			idx = (ndidx << 4) + ofs;
			return m_docindexptr[ ndidx].base + m_docindexptr[ ndidx].ofs[ ofs-1];
		}
		ofs = 0;
		++ndidx;
	}
}

Index PosinfoBlock::skipDoc( const Index& docno_, Cursor& idx) const
{
	unsigned int ndidx = idx >> 4;
	unsigned short ofs = idx & 0xF;

	const DocIndexNode& nd = m_docindexptr[ ndidx];
	if (nd.base >= docno_)
	{
		if (nd.base == docno_)
		{
			return docno_;
		}
		if (ndidx > 0)
		{
			ndidx = 0;
			ofs = 0;
		}
		else
		{
			return nd.base;
		}
	}
	unsigned int fib1 = 1, fib2 = 1;
	unsigned int prev_ndidx = ndidx++;
	while (ndidx < m_nofDocIndexNodes && m_docindexptr[ ndidx].base < docno_)
	{
		prev_ndidx = ndidx;
		unsigned int fibres = fib1 + fib2;
		ndidx += fibres;
		fib1 = fib2;
		fib2 = fibres;
	}
	ndidx = prev_ndidx + 1;
	while (ndidx < m_nofDocIndexNodes && m_docindexptr[ ndidx].base < docno_)
	{
		++ndidx;
	}
	Index rt = m_docindexptr[ --ndidx].skipDoc( docno_, ofs);
	if (rt)
	{
		idx = (ofs + (ndidx << 4));
		return rt;
	}
	else
	{
		idx = 0;
		return 0;
	}
}

Index PosinfoBlock::PositionScanner::skip( const Index& pos)
{
	if (m_itr >= m_size || (Index)m_ar[ m_itr] > pos)
	{
		while (m_itr > 0 && (Index)m_ar[ m_itr>>1] > pos)
		{
			m_itr >>= 1;
		}
	}
	if (pos > (Index)std::numeric_limits<PositionType>::max()) return 0;

	unsigned int fib1 = 1, fib2 = 1;
	unsigned int prev_itr = m_itr++;
	while (m_itr < m_size && (Index)m_ar[ m_itr] < pos)
	{
		prev_itr = m_itr;
		unsigned int fibres = fib1 + fib2;
		m_itr += fibres;
		fib1 = fib2;
		fib2 = fibres;
	}
	m_itr = prev_itr + 1;
	while (m_itr < m_size && (Index)m_ar[ m_itr] < pos)
	{
		++m_itr;
	}
	return (m_itr < m_size)?(Index)m_ar[ m_itr]:0;
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
			throw std::runtime_error( "internal: documents not added in ascending order into posinfo block");
		}
		if (base + std::numeric_limits<unsigned short>::max() < docno_)
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

Index PosinfoBlock::DocIndexNode::skipDoc( const Index& docno_, unsigned short& posrefIdx_) const
{
	if (docno_ <= base)
	{
		if (docno_ == base)
		{
			posrefIdx_ = posrefIdx[0];
			return base;
		}
		return false;
	}
	if (docno_ - base > std::numeric_limits<unsigned short>::max()) return false;
	unsigned short ofs_ = (unsigned short)(docno_ - base);
	std::size_t ii = 0;
	for (; ii<(Size-1) && ofs_ > ofs[ii]; ++ii){}
	if (ii == (Size-1))
	{
		return false;
	}
	else
	{
		posrefIdx_ = posrefIdx[ ii+1];
		return base + ofs[ii];
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

void PosinfoBlockBuilder::append( const Index& docno, const std::vector<Index>& pos)
{
	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
		{
			throw std::logic_error( "corrupt structure in posinfo block builder");
		}
	}
	std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
	if (pos.size() > std::numeric_limits<PositionType>::max())
	{
		throw std::runtime_error( "size of document out of range (max 65535)");
	}
	m_posinfoArray.push_back( (PositionType)pos.size());
	for (; pi != pe; ++pi)
	{
		if (*pi > std::numeric_limits<PositionType>::max())
		{
			throw std::runtime_error( "token position out of range (max 65535)");
		}
		m_posinfoArray.push_back( (PositionType)*pi);
	}
	m_lastDoc = docno;
}

void PosinfoBlockBuilder::append( const Index& docno, const PositionType* posar)
{
	if (m_id && m_id < docno) throw std::runtime_error( "assigned illegal id to block");

	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
		{
			throw std::logic_error( "corrupt structure in posinfo block builder");
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
	if (empty()) throw std::runtime_error( "tried to create empty posinfo block");

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
	if (id_ && id_ < m_lastDoc) throw std::runtime_error( "assigning illegal id to block");
	m_id = id_;
}




