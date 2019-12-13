/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlock.hpp"
#include "memBlock.hpp"
#include "indexPacker.hpp"
#include "strus/base/static_assert.hpp"
#include "strus/base/string_format.hpp"
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
		STRUS_STATIC_ASSERT( sizeof(StructBlock::StructureMemberRange) == sizeof(StructBlock::StructureRepeat));
		STRUS_STATIC_ASSERT( sizeof(StructBlock::StructureDef) == 2*sizeof(StructBlock::PositionType) + 2*sizeof(StructBlock::MemberIdxType));
	}
};
}//anonymous namespace
static StaticAsserts g_staticAsserts;


template <typename Element>
static const Element* getStructPtr( const char* name, const void* dataPtr, int indexStart, int indexEnd)
{
	if (indexStart % AlignmentBase != 0 || (indexEnd-indexStart) % sizeof(Element) != 0 || indexEnd < indexStart)
	{
		std::string dt = strus::string_format( "start: %d, end: %d, alignment base: %d, size of element: %d", indexStart, indexEnd, (int)AlignmentBase, (int)sizeof(Element));
		throw strus::runtime_error( _TXT("data corruption in structure block: misalignment of %s, %s"), name, dt.c_str());
	}
	return (Element*)((char*)dataPtr + indexStart);
}

template <typename Element>
static int getStructSize( const char* name, int indexStart, int indexEnd)
{
	if (0!=(indexEnd-indexStart) % sizeof(Element)) throw strus::runtime_error(_TXT("data corruption in structure block: structure size of %s"), name);
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
		m_enumMemberar = 0;
	}
	else if (size() >= sizeof(BlockHeader))
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		int blockSize = (int)DataBlock::size();

		const DocIndexNode* docIndexNodePtr = getStructPtr<DocIndexNode>( "document index nodes", ptr(), sizeof(BlockHeader), hdr->structlistidx);
		int docIndexNodeSize = getStructSize<DocIndexNode>( "document index nodes", sizeof(BlockHeader), hdr->structlistidx);
		m_docIndexNodeArray.init( docIndexNodePtr, docIndexNodeSize);
		m_structlistar = getStructPtr<StructureDefList>( "structure definition list", ptr(), hdr->structlistidx, hdr->structidx);
		m_structar = getStructPtr<StructureDef>( "structure definitions", ptr(), hdr->structidx, hdr->memberidx);
		m_memberar = getStructPtr<StructureMemberRange>( "member definitions", ptr(), hdr->memberidx, hdr->enumMemberidx);
		m_enumMemberar = getStructPtr<StructureMemberEnum>( "enumeration member definitions", ptr(), hdr->enumMemberidx, blockSize);
	}
	else
	{
		throw strus::runtime_error( _TXT("data corruption in structure block: %s"), _TXT("no header defined"));
	}
}

strus::IndexRange StructBlock::StructureMemberRange::Iterator::next()
{
	return skip( cur.end());
}

strus::IndexRange StructBlock::StructureMemberRange::Iterator::skip( strus::Index pos)
{
	if (aridx >= ar.size())
	{
		if (aridx == 0) return cur = strus::IndexRange();
		aridx = 0;
	}
	if ((Index)ar[ aridx].start == StructureRepeat::ID)
	{
		++aridx;
		if (aridx >= ar.size())
		{
			aridx = 0;
		}
	}
	if ((Index)ar[ aridx].end <= pos)
	{
		int idx = ar.upperbound( pos, aridx, ar.size(), StructureMemberRange::SearchCompare());
		if (idx >= 0) aridx = idx; else return cur = strus::IndexRange();
	}
	if (aridx+1 < ar.size() && (Index)ar[ aridx+1].start == StructureRepeat::ID)
	{
		const StructureRepeat* rep = (const StructureRepeat*)&ar[ aridx+1];
		Index start = ar[ aridx].start;
		Index rpos = (pos < start) ? 0 : (pos - start);
		Index base = rpos - rpos % rep->ofs + start;
		if (base + rep->size > pos)
		{
			return cur = strus::IndexRange( base, base + rep->size);
		}
		else if (base + rep->ofs <= ar[ aridx].end)
		{
			base += rep->ofs;
			return cur = strus::IndexRange( base, base + rep->size);
		}
		else if (aridx+2 < ar.size())
		{
			aridx += 2;
			return cur = strus::IndexRange( ar[ aridx].start, ar[ aridx].end);
		}
		else
		{
			return cur = strus::IndexRange();
		}
	}
	else
	{
		return cur = strus::IndexRange( ar[ aridx].start, ar[ aridx].end);
	}
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& o)
	:m_docIndexNodeArray(),m_structurelistar(),m_structurear(),m_memberar(),m_enumMemberar()
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
		StructureMemberRange* mmb = (StructureMemberRange*)&m_memberar[ m_memberar.size()-2];
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
			m_memberar.push_back( *(StructureMemberRange*)&rep);
			if (m_structurear.back().membersSize >= StructureDef::MaxMembersSize)
			{
				throw std::runtime_error(_TXT("number of structure members exceeds maximum size"));
			}
			++m_structurear.back().membersSize;
			return true;
		}
	}
	return false;
}

bool StructBlockBuilder::tryMoveRangeListBlockMembersToEnumeration()
{
	if (m_structurear.empty()) return false;
	StructureDef& st = m_structurear.back();
	if (st.structureType != StructureDef::TypeRangeList) return false;
	if ((std::size_t)st.membersIdx + st.membersSize != m_memberar.size()) return false;
	StructBlock::MemberScanner mscan( m_memberar.data()+st.membersIdx, st.membersSize);
	std::vector<StructureMemberEnum> enumar;
	std::size_t maxSize = (std::size_t)st.membersSize * sizeof(StructureMemberRange);
	strus::IndexRange rg = mscan.skip( 0);
	for (; rg.defined(); rg = mscan.skip( rg.end()))
	{
		strus::Index ri = rg.start(), re = rg.end();
		for (; ri < re; ++ri)
		{
			if (enumar.empty() || enumar.back().full())
			{
				enumar.push_back( StructureMemberEnum());
			}
			if (!enumar.back().append( ri)) return false;
		}
		if (enumar.size() * sizeof(StructureMemberEnum) >= maxSize) return false;
		if (enumar.size() >= StructureDef::MaxMembersSize) return false;
	}
	st.structureType = StructureDef::TypeEnumList;
	st.membersIdx = m_enumMemberar.size();
	st.membersSize = enumar.size();
	m_enumMemberar.insert( m_enumMemberar.end(), enumar.begin(), enumar.end());
	return true;
}

void StructBlockBuilder::addLastStructureMemberRange( const IndexRange& sink)
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
				addLastStructureMemberRange( joined);
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
	if (m_structurear.back().membersSize >= StructureDef::MaxMembersSize)
	{
		throw std::runtime_error(_TXT("number of structure members exceeds maximum size"));
	}
	m_memberar.push_back( StructureMemberRange( sink.start(), sink.end()));
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

		(void)tryMoveRangeListBlockMembersToEnumeration();
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
				addLastStructureMemberRange( sink);
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
			addLastStructureMemberRange( sink);
		}
	}
	else
	{
		addLastDocStructure( src);
		addLastStructureMemberRange( sink);
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
		const StructureDef& stdef = structDefs[ ni];
		IndexRange source( stdef.header_start, stdef.header_end);
		switch (stdef.structureType)
		{
			case StructureDef::TypeRangeList:
			{
				int nofMembers = stdef.membersSize;
				const StructureMemberRange* memberDefs = m_memberar.data() + stdef.membersIdx;
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
				break;
			}
			case StructureDef::TypeEnumList:
			{
				StructureMemberEnum::Iterator itr( m_enumMemberar.data() + stdef.membersIdx, stdef.membersSize);
				strus::IndexRange sink = itr.next();
				for (; sink.defined(); sink = itr.next())
				{
					append( docno, source, sink);
				}
				break;
			}
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
	int estimatedConsumption = nofstructures * (sizeof(StructureMemberRange));
	return size() + estimatedConsumption <= Constants::maxStructBlockSize();
}

StructBlock StructBlockBuilder::createBlock()
{
	if (empty()) throw strus::runtime_error( _TXT("tried to create empty structure block"));
	(void)tryMoveRangeListBlockMembersToEnumeration();

	StructBlock::BlockHeader hdr;

	int docindexofs = sizeof( StructBlock::BlockHeader);
	int structlistofs = docindexofs + m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);
	int structofs = structlistofs + m_structurelistar.size() * sizeof( m_structurelistar[0]);
	int memberofs = structofs + m_structurear.size() * sizeof( m_structurear[0]);
	int enumMemberofs = memberofs + m_memberar.size() * sizeof( m_memberar[0]);
	int blksize = enumMemberofs + m_enumMemberar.size() * sizeof( m_enumMemberar[0]);

	int first_docno = m_docIndexNodeArray[0].firstDoc();
	int last_docno = m_docIndexNodeArray.back().lastDoc();

	if (m_enumMemberar.size() > (std::size_t)std::numeric_limits<StructBlock::MemberIdxType>::max())
	{
		throw strus::runtime_error(_TXT("number of members (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_enumMemberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
	}
	if (m_memberar.size() > (std::size_t)std::numeric_limits<StructBlock::MemberIdxType>::max())
	{
		throw strus::runtime_error(_TXT("number of members (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_memberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
	}
	if (m_structurear.size() > (std::size_t)std::numeric_limits<StructBlock::StructIdxType>::max())
	{
		throw strus::runtime_error(_TXT("number of structures (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_structurear.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::StructIdxType>::max());
	}
	if (m_structurelistar.size() > (std::size_t)std::numeric_limits<unsigned short>::max()/*element of DocIndexNode::ref*/)
	{
		throw strus::runtime_error(_TXT("number of structure lists (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_structurelistar.size(), first_docno, last_docno, (int)std::numeric_limits<unsigned short>::max());
	}
	hdr.structlistidx = structlistofs;
	hdr.structidx = structofs;
	hdr.memberidx = memberofs;
	hdr.enumMemberidx = enumMemberofs;

	MemBlock blkmem( blksize);
	char* dt = (char*)blkmem.ptr();
	std::memcpy( dt, &hdr, sizeof(hdr));
	std::memcpy( dt+docindexofs, m_docIndexNodeArray.data(), m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]));
	std::memcpy( dt+structlistofs, m_structurelistar.data(), m_structurelistar.size() * sizeof( m_structurelistar[0]));
	std::memcpy( dt+structofs, m_structurear.data(), m_structurear.size() * sizeof( m_structurear[0]));
	std::memcpy( dt+memberofs, m_memberar.data(), m_memberar.size() * sizeof( m_memberar[0]));
	std::memcpy( dt+enumMemberofs, m_enumMemberar.data(), m_enumMemberar.size() * sizeof( m_enumMemberar[0]));

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
	m_enumMemberar.clear();
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
	if (cursor.stutype == StructureDef::TypeRangeList && cursor.mbridx+1 < cursor.mbrend && m_memberar[ cursor.mbridx+1].start == StructureRepeat::ID)
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
	switch (cursor.stutype)
	{
		case StructureDef::TypeRangeList:
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
		case StructureDef::TypeEnumList:
			src = strus::IndexRange(
				m_structurear[ cursor.stuidx].header_start,
				m_structurear[ cursor.stuidx].header_end);
			sink = cursor.mitr.current();
			return sink.defined() ? cursor.docno : 0;
			
	}
	return 0;
}

strus::Index StructBlockBuilder::nextMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	switch (cursor.stutype)
	{
		case StructureDef::TypeRangeList:
			++cursor.mbridx;
			if (firstRepeatNode( cursor, src, sink))
			{
				return cursor.docno;
			}
			else
			{
				return currentMemberNode( cursor, src, sink);
			}
		case StructureDef::TypeEnumList:
			sink = cursor.mitr.next();
			if (sink.defined()) return cursor.docno;
	}
	return 0;
}

strus::Index StructBlockBuilder::nextStructureFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
{
	for (++cursor.stuidx; cursor.stuidx < cursor.stuend; ++cursor.stuidx)
	{
		const StructureDef& stdef = m_structurear[ cursor.stuidx];
		cursor.stutype = (StructureDef::StructureType)stdef.structureType;
		cursor.mbridx = stdef.membersIdx;
		cursor.mbrend = cursor.mbridx + stdef.membersSize;
		if (cursor.mbridx < cursor.mbrend)
		{
			switch (cursor.stutype)
			{
				case StructureDef::TypeRangeList:
					if (firstRepeatNode( cursor, src, sink))
					{
						return cursor.docno;
					}
					else
					{
						return currentMemberNode( cursor, src, sink);
					}
				case StructureDef::TypeEnumList:
					cursor.mitr = StructureMemberEnum::Iterator( m_enumMemberar.data()+cursor.mbridx, cursor.mbrend-cursor.mbridx);
					sink = cursor.mitr.next();
					if (sink.defined()) return cursor.docno;
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
