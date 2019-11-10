/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

std::vector<Index> PosinfoBlock::positions_at( const DocIndexNodeCursor& cursor) const
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

unsigned int PosinfoBlock::frequency_at( const DocIndexNodeCursor& cursor) const
{
	const DocIndexNode& nd = m_docIndexNodeArray[ cursor];
	return m_posinfoptr[ nd.ref[ cursor.docidx]];
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

void PosinfoBlock::initFrame()
{
	if (size() < sizeof(unsigned int))
	{
		m_docIndexNodeArray.init( 0, 0);
		m_posinfoptr = 0;
	}
	else
	{
		int nofDocIndexNodes = *(const unsigned int*)ptr();
		const DocIndexNode* docIndexNodes = (const DocIndexNode*)( (const unsigned int*)ptr()+1);
		m_docIndexNodeArray.init( docIndexNodes, nofDocIndexNodes);
		m_posinfoptr = (const PositionType*)(const void*)(docIndexNodes + nofDocIndexNodes);
	}
}


PosinfoBlockBuilder::PosinfoBlockBuilder( const PosinfoBlock& o)
	:m_id(o.id())
{
	DocIndexNodeCursor idx;
	Index docno;
	if (!(docno=o.firstDoc( idx))) return;
	do
	{
		append( docno, o.posinfo_at( idx));
	}
	while (!!(docno=o.nextDoc( idx)));
}

void PosinfoBlockBuilder::append( const Index& docno, const PositionType* posar)
{
	if (m_id && m_id < docno) throw std::runtime_error(_TXT("assigned illegal id to block"));

	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_posinfoArray.size()))
		{
			throw std::runtime_error( _TXT("corrupt structure in posinfo block builder"));
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
	if (empty()) throw std::runtime_error(_TXT("tried to create empty posinfo block"));

	std::size_t blksize =
		sizeof( unsigned int)
		+ m_posinfoArray.size() * sizeof( m_posinfoArray[0])
		+ m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);

	MemBlock blkmem( blksize);
	unsigned int nofDocIndexNodes = *(unsigned int*)blkmem.ptr() = docIndexNodeArray().size();
	DocIndexNode* docindexptr = (DocIndexNode*)( (const unsigned int*)blkmem.ptr()+1);
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
	return PosinfoBlock( m_id?m_id:m_lastDoc, blkmem.ptr(), blksize, true/*allocated, means copied*/);
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
	if (id_ && id_ < m_lastDoc) throw std::runtime_error(_TXT("assigning illegal id to block"));
	m_id = id_;
}




