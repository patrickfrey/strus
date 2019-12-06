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
#include "strus/base/static_assert.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>

using namespace strus;

namespace {
struct _AlignmentBaseStruct {char _;};
#define AlignmentBase sizeof(_AlignmentBaseStruct)

struct StaticAsserts
{
	StaticAsserts()
	{
		STRUS_STATIC_ASSERT( sizeof(StructBlock::StructureMember) == sizeof(StructBlock::StructureRepeat));
	}
};
}//anonymous namespace
static StaticAsserts g_staticAsserts;


template <typename Element>
static const Element* getStructPtr( const void* dataPtr, unsigned short indexStart, unsigned short indexEnd)
{
	if (indexStart % AlignmentBase != 0 || (indexEnd-indexStart) % sizeof(Element) != 0 || indexEnd < indexStart)
	{
		throw std::runtime_error( _TXT("data corruption in structure block"));
	}
	return (Element*)((char*)dataPtr + indexStart);
}

template <typename Element>
static int getStructSize( unsigned short indexStart, unsigned short indexEnd)
{
	if (0!=(indexEnd-indexStart) % sizeof(Element)) throw std::runtime_error(_TXT("data corruption in structure block"));
	return (indexEnd-indexStart) / sizeof(Element);
}

void StructBlock::initFrame()
{
	if (empty())
	{
		m_docIndexNodeArray.init( 0, 0);
		m_structlistar = 0;
		m_structar = 0;
		m_memberar = 0;
	}
	else if (size() >= sizeof(BlockHeader))
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		unsigned short blockSize = (int)DataBlock::size();

		const DocIndexNode* docIndexNodePtr = getStructPtr<DocIndexNode>( ptr(), sizeof(BlockHeader), hdr->structlistidx);
		int docIndexNodeSize = getStructSize<DocIndexNode>( sizeof(BlockHeader), hdr->structlistidx);
		m_docIndexNodeArray.init( docIndexNodePtr, docIndexNodeSize);
		m_structlistar = getStructPtr<StructureDefList>( ptr(), hdr->structlistidx, hdr->structidx);
		m_structar = getStructPtr<StructureDef>( ptr(), hdr->structidx, hdr->memberidx);
		m_memberar = getStructPtr<StructureMember>( ptr(), hdr->memberidx, blockSize);
	}
	else
	{
		throw std::runtime_error( _TXT("data corruption in structure block"));
	}
}

strus::IndexRange StructBlock::MemberScanner::skip( Index pos)
{
	if (m_itr >= m_ar.size())
	{
		if (m_itr == 0) return strus::IndexRange();
		m_itr = 0;
	}
	if ((Index)m_ar[ m_itr].start == StructureRepeat::ID)
	{
		++m_itr;
		if (m_itr >= m_ar.size())
		{
			m_itr = 0;
		}
	}
	if ((Index)m_ar[ m_itr].end <= pos)
	{
		int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureMemberSearchCompare());
		if (idx >= 0) m_itr = idx; else return strus::IndexRange();
	}
	if (m_itr+1 < m_ar.size() && (Index)m_ar[ m_itr+1].start == StructureRepeat::ID)
	{
		const StructureRepeat* rep = (const StructureRepeat*)&m_ar[ m_itr+1];
		Index start = m_ar[ m_itr].start;
		Index rpos = (pos < start) ? 0 : (pos - start);
		Index base = rpos - rpos % rep->ofs + start;
		if (base + rep->size > pos)
		{
			return strus::IndexRange( base, base + rep->size);
		}
		else if (base + rep->ofs <= m_ar[ m_itr].end)
		{
			base += rep->ofs;
			return strus::IndexRange( base, base + rep->size);
		}
		else if (m_itr+2 < m_ar.size())
		{
			m_itr += 2;
			return strus::IndexRange( m_ar[ m_itr].start, m_ar[ m_itr].end);
		}
		else
		{
			return strus::IndexRange();
		}
	}
	else
	{
		return strus::IndexRange( m_ar[ m_itr].start, m_ar[ m_itr].end);
	}
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& o)
	:m_docIndexNodeArray(),m_memberar(),m_structurelistar(),m_structurear()
	,m_lastDoc(0),m_id(0)
{
	DocIndexNodeCursor cursor;
	strus::Index docno = o.firstDoc( cursor);
	while (docno)
	{
		appendFromBlock( docno, o, cursor);
		docno = o.nextDoc( cursor);
	}
	m_id = o.id();
}

void StructBlockBuilder::addNewDocument( Index docno)
{
	if (m_docIndexNodeArray.empty()
	||  !m_docIndexNodeArray.back().addDocument( docno, m_structurelistar.size()))
	{
		m_docIndexNodeArray.push_back( DocIndexNode());
		if (!m_docIndexNodeArray.back().addDocument( docno, m_structurelistar.size()))
		{
			throw std::runtime_error( _TXT("corrupt structure in structure block builder"));
		}
	}
	StructureDefList lst;
	lst.idx = m_structurear.size();
	lst.size = 0;
	m_structurelistar.push_back( lst);
	m_lastDoc = docno;
}

bool StructBlockBuilder::addFittingRepeatMember( const strus::IndexRange& sink)
{
	if (!m_structurear.back().membersSize) return false;
	if ((Index)m_memberar.back().start == StructureRepeat::ID)
	{
		if (m_structurear.back().membersSize < 2 || m_memberar.size() < 2) throw std::runtime_error(_TXT("corrupt index: repeat structure not following a member describing the structure area"));
		StructureMember* mmb = (StructureMember*)&m_memberar[ m_memberar.size()-2];
		const StructureRepeat* rep = (const StructureRepeat*)&m_memberar.back();
		strus::Index membsize = sink.end() - sink.start();
		strus::Index membofs = sink.end() - mmb->end;

		if (membsize == (Index)(int)(unsigned int)rep->size
		&&  membofs  == (Index)(int)(unsigned int)rep->ofs)
		{
			mmb->end += membofs;
			return true;
		}
	}
	else
	{
		strus::Index membsize = sink.end() - sink.start();
		strus::Index membofs = sink.end() - m_memberar.back().end;

		if (membsize == m_memberar.back().end - m_memberar.back().start
		&&  membsize < (Index)std::numeric_limits<unsigned char>::max()
		&&  membofs  < (Index)std::numeric_limits<unsigned char>::max())
		{
			StructureRepeat rep( membofs, membsize);
			m_memberar.back().end += membofs;
			m_memberar.push_back( *(StructureMember*)&rep);
			++m_structurear.back().membersSize;
			return true;
		}
	}
	return false;
}

void StructBlockBuilder::addLastStructureMember( const IndexRange& sink)
{
	if (m_structurear.back().membersSize)
	{
		if ((Index)m_memberar.back().start != StructureRepeat::ID
		&&  (Index)m_memberar.back().end >= sink.start())
		{
			if ((Index)m_memberar.back().end == sink.start())
			{
				// ... joining subsequent structure members to one
				IndexRange joined( m_memberar.back().start, sink.end());
				m_memberar.pop_back();
				--m_structurear.back().membersSize,
				addLastStructureMember( joined);
				return;
			}
			else
			{
				throw strus::runtime_error( _TXT("structure members in structure block builder not appended in strictly ascending order"));
			}
		}
		else if (addFittingRepeatMember( sink))
		{
			return;
		}
	}
	m_memberar.push_back( StructureMember( sink.start(), sink.end()));
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

void StructBlockBuilder::append( Index docno, const IndexRange& src, const IndexRange& sink)
{
	if (docno <= 0)
	{
		throw std::runtime_error(_TXT("cannot add docno <= 0"));
	}
	if (src.start() >= src.end() || sink.start() >= sink.end())
	{
		throw std::runtime_error(_TXT("adding empty structures not allowed"));
	}
	if (m_lastDoc > docno)
	{
		throw strus::runtime_error(_TXT("documents not added in ascending order (%d > %d)"), (int)m_lastDoc, (int)docno);
	}
	if (m_lastDoc != docno)
	{
		if (m_id && m_id < docno) throw strus::runtime_error( _TXT("assigned illegal id to block"));
		addNewDocument( docno);
	}
	if (m_structurelistar.back().size)
	{
		if (src.start() < (Index)m_structurear.back().header_start)
		{
			throw strus::runtime_error( _TXT("structures in structure block builder not appended in ascending order"));
		}
		else if (src.start() == (Index)m_structurear.back().header_start)
		{
			if (src.end() == (Index)m_structurear.back().header_end)
			{
				addLastStructureMember( sink);
			}
			else
			{
				throw strus::runtime_error( _TXT("overlaping sources of structures not equal in structure block builder"));
			}
		}
		else if (src.start() < (Index)m_structurear.back().header_end)
		{
			throw strus::runtime_error( _TXT("overlaping sources of structures not equal in structure block builder"));
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

void StructBlockBuilder::merge( const StructBlockBuilder& blk1, const StructBlockBuilder& blk2, StructBlockBuilder& newblk)
{
	Cursor cursor1;
	strus::IndexRange src1;
	strus::IndexRange sink1;
	strus::Index docno1 = blk1.firstNode( cursor1, src1, sink1);

	Cursor cursor2;
	strus::IndexRange src2;
	strus::IndexRange sink2;
	strus::Index docno2 = blk2.firstNode( cursor2, src2, sink2);

	while (docno1 && docno2)
	{
		if (docno1 < docno2)
		{
			newblk.append( docno1, src1, sink1);
			docno1 = blk1.nextNode( cursor1, src1, sink1);
		}
		else if (docno1 > docno2)
		{
			newblk.append( docno2, src2, sink2);
			docno2 = blk2.nextNode( cursor2, src2, sink2);
		}
		else/*(docno1 == docno2)*/
		{
			throw std::runtime_error(_TXT("cannot merge structure blocks with overlapping document references"));
		}
	}
	while (docno1)
	{
		newblk.append( docno1, src1, sink1);
		docno1 = blk1.nextNode( cursor1, src1, sink1);
	}
	while (docno2)
	{
		newblk.append( docno2, src2, sink2);
		docno2 = blk2.nextNode( cursor2, src2, sink2);
	}
}

void StructBlockBuilder::merge(
		std::vector<StructDeclaration>::const_iterator ei,
		const std::vector<StructDeclaration>::const_iterator& ee,
		const StructBlock& oldblk,
		StructBlockBuilder& newblk)
{
	newblk.clear();
	newblk.setId( oldblk.id());
	merge_append( ei, ee, oldblk, newblk);
}

void StructBlockBuilder::appendFromBlock( Index docno, const StructBlock& blk, const DocIndexNodeCursor& cursor)
{
	int nofStructs;
	const StructureDef* structDefs = blk.structures_at( cursor, nofStructs);
	int ni = 0, ne = nofStructs;
	for (; ni != ne; ++ni)
	{
		IndexRange source( structDefs[ ni].header_start, structDefs[ ni].header_end);
		int nofMembers;
		const StructureMember* memberDefs = blk.members_at( cursor, structDefs+ni, nofMembers);
		int mi = 0, me = nofMembers;
		if (me)
		{
			IndexRange sink( memberDefs[ mi].start, memberDefs[ mi].end);
			append( docno, source, sink);
		}
		for (++mi; mi != me; ++mi)
		{
			m_memberar.push_back( memberDefs[ mi]);
			++m_structurear.back().membersSize;
		}
	}
}

void StructBlockBuilder::merge_append(
		std::vector<StructDeclaration>::const_iterator ei,
		const std::vector<StructDeclaration>::const_iterator& ee,
		const StructBlock& oldblk,
		StructBlockBuilder& appendblk)
{
	strus::IndexRange src;
	strus::IndexRange sink;
	DocIndexNodeCursor cursor;

	strus::Index docno = oldblk.firstDoc( cursor);
	while (docno && ei != ee && ei->docno <= oldblk.id())
	{
		if (ei->docno < docno)
		{
			strus::Index first_docno = ei->docno;
			while (ei != ee && ei->docno == first_docno)
			{
				if (ei->sink.defined() || ei->src.defined())
				{
					appendblk.append( ei->docno, ei->src, ei->sink);
				}
				ei++;
			}
		}
		else if (ei->docno > docno)
		{
			appendblk.appendFromBlock( docno, oldblk, cursor);
			docno = oldblk.nextDoc( cursor);
		}
		else/*(ei->docno == docno)*/
		{
			strus::Index first_docno = ei->docno;
			while (ei != ee && ei->docno == first_docno)
			{
				if (ei->sink.defined() || ei->src.defined())
				{
					appendblk.append( ei->docno, ei->src, ei->sink);
				}
				ei++;
			}
			while (docno == first_docno)
			{
				// ... skip old block elements, they are replaced
				docno = oldblk.nextDoc( cursor);
			}
		}
	}
	while (docno)
	{
		appendblk.appendFromBlock( docno, oldblk, cursor);
		docno = oldblk.nextDoc( cursor);
	}
	while (ei != ee)
	{
		if (ei->sink.defined() || ei->src.defined())
		{
			appendblk.append( ei->docno, ei->src, ei->sink);
		}
		++ei;
	}
}

void StructBlockBuilder::split( const StructBlockBuilder& blk, StructBlockBuilder& newblk1, StructBlockBuilder& newblk2)
{
	std::vector<StructDeclaration> declar;
	Cursor cursor;
	strus::IndexRange src;
	strus::IndexRange sink;
	strus::Index docno = blk.firstNode( cursor, src, sink);
	while (docno)
	{
		declar.push_back( StructDeclaration( docno, src, sink));
		docno = blk.nextNode( cursor, src, sink);
	}
	newblk1.clear();
	newblk2.clear();
	if (declar.empty()) return;

	std::size_t splitidx = declar.size() / 2 - declar.size() / 16;
	docno = declar[ splitidx].docno;
	while (splitidx < declar.size() && declar[splitidx].docno == docno) {++splitidx;}
	if (splitidx == declar.size())
	{
		while (splitidx > 0 && declar[splitidx-1].docno == docno) {--splitidx;}
		if (splitidx == 0)
		{
			newblk1 = blk;
			return;
		}
	}
	std::vector<StructDeclaration>::const_iterator di = declar.begin(), de = declar.begin()+splitidx;
	for (; di != de; ++di)
	{
		newblk1.append( di->docno, di->src, di->sink);
	}
	di = declar.begin()+splitidx, de = declar.end();
	for (; di != de; ++di)
	{
		newblk2.append( di->docno, di->src, di->sink);
	}
}

bool StructBlockBuilder::fitsInto( std::size_t nofstructures) const
{
	int estimatedConsumption = nofstructures * (sizeof(StructureMember));
	return size() + estimatedConsumption <= Constants::maxStructBlockSize();
}

StructBlock StructBlockBuilder::createBlock() const
{
	if (empty()) throw strus::runtime_error( _TXT("tried to create empty structure block"));

	StructBlock::BlockHeader hdr;

	int docindexofs = sizeof( StructBlock::BlockHeader);
	int structlistofs = docindexofs + m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);
	int structofs = structlistofs + m_structurelistar.size() * sizeof( m_structurelistar[0]);
	int memberofs = structofs + m_structurear.size() * sizeof( m_structurear[0]);
	int blksize = memberofs + m_memberar.size() * sizeof( m_memberar[0]);
	if (blksize > (int)std::numeric_limits<strus::Index>::max())
	{
		int first_docno = m_docIndexNodeArray[0].firstDoc();
		int last_docno = m_docIndexNodeArray.back().lastDoc();
		throw strus::runtime_error(_TXT("sizeof block (%d) for documents [%d,%d] exceeds maximum limit %d"), blksize, first_docno, last_docno, (int)std::numeric_limits<strus::Index>::max());
	}
	hdr.structlistidx = structlistofs;
	hdr.structidx = structofs;
	hdr.memberidx = memberofs;
	hdr._RESERVED = 0;

	MemBlock blkmem( blksize);
	char* dt = (char*)blkmem.ptr();
	std::memcpy( dt, &hdr, sizeof(hdr));
	std::memcpy( dt+docindexofs, m_docIndexNodeArray.data(), m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]));
	std::memcpy( dt+structlistofs, m_structurelistar.data(), m_structurelistar.size() * sizeof( m_structurelistar[0]));
	std::memcpy( dt+structofs, m_structurear.data(), m_structurear.size() * sizeof( m_structurear[0]));
	std::memcpy( dt+memberofs, m_memberar.data(), m_memberar.size() * sizeof( m_memberar[0]));

	return StructBlock( m_id?m_id:m_lastDoc, blkmem.ptr(), blksize, true);
}

void StructBlockBuilder::setId( Index id_)
{
	if (id_ && id_ < m_lastDoc) throw strus::runtime_error( _TXT("assigning illegal id to block"));
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

strus::Index StructBlockBuilder::currentRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	src = strus::IndexRange(
		m_structurear[ cursor.stuidx].header_start,
		m_structurear[ cursor.stuidx].header_end);
	sink = IndexRange( cursor.repstart, cursor.repstart + cursor.repsize);
	return cursor.docno;
}

strus::Index StructBlockBuilder::firstRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (cursor.mbridx+1 < cursor.mbrend && m_memberar[ cursor.mbridx+1].start == StructureRepeat::ID)
	{
		cursor.repstart = m_memberar[ cursor.mbridx].start;
		cursor.repend = m_memberar[ cursor.mbridx].end;
		++cursor.mbridx;
		const StructureRepeat* rep = (const StructureRepeat*)&m_memberar[ cursor.mbridx];
		cursor.repofs = (Index)(int)(unsigned int)rep->ofs;
		cursor.repsize = (Index)(int)(unsigned int)rep->size;
		return currentRepeatNode( cursor, src, sink);
	}
	return 0;
}

strus::Index StructBlockBuilder::nextRepeatNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (cursor.repstart < cursor.repend)
	{
		cursor.repstart += cursor.repofs;
		if (cursor.repstart >= cursor.repend)
		{
			cursor.repstart = 0;
			cursor.repend = 0;
			cursor.repofs = 0;
			cursor.repsize = 0;
		}
		else
		{
			return currentRepeatNode( cursor, src, sink);
		}
	}
	return 0;
}

strus::Index StructBlockBuilder::currentMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (cursor.mbridx >= cursor.mbrend)
	{
		return 0;
	}
	else
	{
		src = strus::IndexRange(
			m_structurear[ cursor.stuidx].header_start,
			m_structurear[ cursor.stuidx].header_end);
		sink = strus::IndexRange(
			m_memberar[ cursor.mbridx].start,
			m_memberar[ cursor.mbridx].end);
	}
	return cursor.docno;
}

strus::Index StructBlockBuilder::nextMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	++cursor.mbridx;
	if (firstRepeatNode( cursor, src, sink))
	{
		return cursor.docno;
	}
	else
	{
		return currentMemberNode( cursor, src, sink);
	}
}

strus::Index StructBlockBuilder::nextStructureFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	for (++cursor.stuidx; cursor.stuidx < cursor.stuend; ++cursor.stuidx)
	{
		cursor.mbridx = m_structurear[ cursor.stuidx].membersIdx;
		cursor.mbrend = cursor.mbridx + m_structurear[ cursor.stuidx].membersSize;
		if (cursor.mbridx < cursor.mbrend)
		{
			if (firstRepeatNode( cursor, src, sink))
			{
				return cursor.docno;
			}
			else
			{
				return currentMemberNode( cursor, src, sink);
			}
		}
	}
	return 0;
}

strus::Index StructBlockBuilder::resolveDocFirstStructure( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	strus::Index ref = m_docIndexNodeArray[ cursor.aridx].ref[ cursor.docidx];
	cursor.stuidx = m_structurelistar[ ref].idx;
	cursor.stuend = cursor.stuidx + m_structurelistar[ ref].size;
	--cursor.stuidx;
	return nextStructureFirstNode( cursor, src, sink);
}

strus::Index StructBlockBuilder::nextDocFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	cursor.docno = m_docIndexNodeArray[ cursor.aridx].nextDoc( cursor.docidx);
	return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
}

strus::Index StructBlockBuilder::nextIndexFirstDoc( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	for (++cursor.aridx; cursor.aridx < m_docIndexNodeArray.size(); ++cursor.aridx)
	{
		cursor.docno = m_docIndexNodeArray[ cursor.aridx].firstDoc( cursor.docidx);
		return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
	}
	return 0;
}

strus::Index StructBlockBuilder::firstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	cursor.aridx = -1;
	return nextIndexFirstDoc( cursor, src, sink);
}

strus::Index StructBlockBuilder::nextNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	if (nextRepeatNode( cursor, src, sink)) return cursor.docno;
	if (nextMemberNode( cursor, src, sink)) return cursor.docno;
	if (nextStructureFirstNode( cursor, src, sink)) return cursor.docno;
	if (nextDocFirstNode( cursor, src, sink)) return cursor.docno;
	if (nextIndexFirstDoc( cursor, src, sink)) return cursor.docno;
	return 0;
}
