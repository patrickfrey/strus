/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlock.hpp"
#include "memBlock.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <iostream>

using namespace strus;

void StructBlock::initFrame()
{
	if (size() < sizeof(BlockHeader))
	{
		m_docIndexNodeArray.init( 0, 0);
		m_structlistar = 0;
		m_structar = 0;
		m_memberar = 0;
	}
	else
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		unsigned short docindexidx = sizeof(BlockHeader);
		if (hdr->docindexsize > (int)DataBlock::size() / sizeof( DocIndexNode)
		||  hdr->structlistidx < (int)sizeof(BlockHeader) || hdr->structlistidx > (int)DataBlock::size() / sizeof(StructureDefList)
		||  hdr->structidx < (int)sizeof(BlockHeader) || hdr->structidx > (int)DataBlock::size() / sizeof(StructureDef)
		||  hdr->memberidx < (int)sizeof(BlockHeader) || hdr->memberidx > (int)DataBlock::size() / sizeof(StructureMember))
		{
			throw strus::runtime_error( _TXT( "data corruption in structure block"));
		}
		m_docIndexNodeArray.init( (const DocIndexNode*)data_at( docindexidx), hdr->docindexsize);
		m_structlistar = (const StructureDefList*)data_at( hdr->structlistidx);
		m_structar = (const StructureDef*)data_at( hdr->structidx);
		m_memberar = (const StructureMember*)data_at( hdr->memberidx);
	}
}

void StructBlockBuilder::addNewDocument( const Index& docno)
{
	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_structurelistar.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_structurelistar.size()))
		{
			throw strus::logic_error( _TXT( "corrupt structure in structure block builder"));
		}
	}
	StructureDefList lst;
	lst.idx = m_structurear.size();
	lst.size = 0;
	m_structurelistar.push_back( lst);
	m_lastDoc = docno;
}

void StructBlockBuilder::addLastStructureMember( const IndexRange& sink)
{
	StructureMember member;
	if (m_structurear.back().membersSize)
	{
		if ((Index)m_memberar.back().end > sink.start())
		{
			throw strus::logic_error( _TXT( "structure members in structure block builder not appended in strictly ascending order"));
		}
	}
	member.start = sink.start();
	member.end = sink.end();
	m_memberar.push_back( member);
	++m_structurear.back().membersSize;
}

void StructBlockBuilder::addLastDocStructure( const IndexRange& src)
{
	StructureDef st;
	st.header_start = src.start();
	st.header_end = src.end();
	st.membersIdx = m_memberar.size();
	st.membersSize = 0;
	m_structurear.push_back( st);
	++m_structurelistar.back().size;
}

void StructBlockBuilder::push( const Index& docno, const IndexRange& src, const IndexRange& sink)
{
	if (!docno) throw std::runtime_error(_TXT("cannot add docno 0"));
	if (m_lastDoc > docno) throw std::runtime_error(_TXT("documents not added in ascending order"));
	if (m_lastDoc != docno)
	{
		if (m_id && m_id < docno) throw strus::runtime_error( "%s",  _TXT( "assigned illegal id to block"));
		addNewDocument( docno);
	}
	if (m_structurelistar.back().size)
	{
		if (src.start() < (Index)m_structurear.back().header_start)
		{
			throw strus::logic_error( _TXT( "structures in structure block builder not appended in ascending order"));
		}
		else if (src.start() == (Index)m_structurear.back().header_start)
		{
			if (src.end() == (Index)m_structurear.back().header_end)
			{
				addLastStructureMember( sink);
			}
			else
			{
				throw strus::logic_error( _TXT( "overlaping sources of structures not equal in structure block builder"));
			}
		}
		else if (src.start() < (Index)m_structurear.back().header_end)
		{
			throw strus::logic_error( _TXT( "overlaping sources of structures not equal in structure block builder"));
		}
		else
		{
			addLastDocStructure( src);
			addLastStructureMember( sink);
		}
	}
	else
	{
		addLastDocStructure( src);
		addLastStructureMember( sink);
	}
}

bool StructBlockBuilder::fitsInto( std::size_t nofstructures) const
{
	return m_memberar.size() + nofstructures <= std::numeric_limits<short>::max();
}

StructBlock StructBlockBuilder::createBlock() const
{
	if (empty()) throw strus::runtime_error( "%s",  _TXT( "tried to create empty structure block"));

	StructBlock::BlockHeader hdr;

	int docindexofs = sizeof( StructBlock::BlockHeader);
	int structlistofs = docindexofs + m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);
	int structofs = structlistofs + m_structurelistar.size() * sizeof( m_structurelistar[0]);
	int memberofs = structofs + m_structurear.size() * sizeof( m_structurear[0]);
	int blksize = memberofs + m_memberar.size() * sizeof( m_memberar[0]);
	if (blksize > std::numeric_limits<unsigned short>::max()) throw strus::runtime_error(_TXT("sizeof block (%d) exceeds maximum limit %d"), blksize, (int)std::numeric_limits<unsigned short>::max());

	hdr.docindexsize = m_docIndexNodeArray.size();
	hdr.structlistidx = structlistofs;
	hdr.structidx = structofs;
	hdr.memberidx = memberofs;

	MemBlock blkmem( blksize);
	char* dt = (char*)blkmem.ptr();
	std::memcpy( dt, &hdr, sizeof(hdr));
	std::memcpy( dt+docindexofs, m_docIndexNodeArray.data(), m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]));
	std::memcpy( dt+structlistofs, m_structurelistar.data(), m_structurelistar.size() * sizeof( m_structurelistar[0]));
	std::memcpy( dt+structofs, m_structurear.data(), m_structurear.size() * sizeof( m_structurear[0]));
	std::memcpy( dt+memberofs, m_memberar.data(), m_memberar.size() * sizeof( m_memberar[0]));

	return StructBlock( m_id?m_id:m_lastDoc, blkmem.ptr(), blksize, true);
}

void StructBlockBuilder::setId( const Index& id_)
{
	if (id_ && id_ < m_lastDoc) throw strus::runtime_error( "%s",  _TXT( "assigning illegal id to block"));
	m_id = id_;
}

void StructBlockBuilder::clear()
{
	m_docIndexNodeArray.clear();
	m_structurelistar.clear();
	m_structurear.clear();
	m_memberar.clear();
	m_lastDoc = 0;
	m_id = 0;
}

