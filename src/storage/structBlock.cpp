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
		STRUS_STATIC_ASSERT( sizeof(StructBlock::StructureMember) == sizeof(StructBlock::PositionType)+ sizeof(unsigned short));
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
		m_startar = 0;
		m_enumar = 0;
		m_repeatar = 0;
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
		m_memberar = getStructPtr<StructureMember>( "member definitions", ptr(), hdr->memberidx, hdr->startidx);
		m_enumar = getStructPtr<StructBlockMemberEnum>( "enum definitions", ptr(), hdr->enumidx, hdr->repeatidx);
		m_repeatar = getStructPtr<StructBlockMemberRepeat>( "enumeration member definitions", ptr(), hdr->repeatidx, blockSize);
		m_startar = getStructPtr<PositionType>( "start definitions", ptr(), hdr->startidx, hdr->enumidx);
	}
	else
	{
		throw strus::runtime_error( _TXT("data corruption in structure block: %s"), _TXT("header not defined defined"));
	}
}

strus::IndexRange StructBlock::StructureMember::Iterator::skip( strus::Index pos)
{
	strus::IndexRange rt;
	if (pos < cur.end() && pos >= cur.start())
	{
		return cur;
	}
	if (aridx >= ar.size())
	{
		if (aridx == 0) return cur = strus::IndexRange();
		aridx = 0;
	}
	if (ar[ aridx].end() <= pos)
	{
		int idx = ar.upperbound( pos, aridx+1, ar.size(), StructureMember::SearchCompare());
		if (idx >= 0) aridx = idx; else return cur = strus::IndexRange();
	}
	else
	{
		int idx = ar.upperbound( pos, 0, aridx, StructureMember::SearchCompare());
		if (idx >= 0) aridx = idx; else return cur = strus::IndexRange();
	}
	const StructureMember& ths = ar[ aridx];
	switch (ths.memberType())
	{
		case StartOffsetType:
			rt = strus::IndexRange( ths.end - ths.memberIdx(), ths.end);
			break;
		case MemberIndexType:
			rt = strus::IndexRange( block->startar()[ ths.memberIdx()], ths.end);
			break;
		case MemberEnumType:
			rt = block->enumar()[ ths.memberIdx()].skip( pos);
			break;
		case MemberRepeatType:
			rt = block->repeatar()[ ths.memberIdx()].skip( pos);
			if (rt.end() > ar[ aridx].end()) rt = strus::IndexRange();
			break;
	}
	return rt;
}

StructBlockBuilder::StructBlockBuilder( const StructBlock& o)
	:m_docIndexNodeArray(),m_structurelistar(),m_structurear(),m_memberar()
	,m_enumar(),m_repeatar(),m_startar(),m_curmembers()
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

void StructBlockBuilder::packCurrentMembers()
{
	
}

void StructBlockBuilder::addLastStructureMemberRange( const IndexRange& sink)
{
	if (!m_curmembers.empty() && m_curmembers.back().end() >= sink.start())
	{
		if (m_curmembers.back().end() == sink.start())
		{
			m_curmembers.back() = strus::IndexRange( m_curmembers.back().start(), sink.end());
		}
		else
		{
			throw strus::runtime_error( _TXT("overlaping sinks of structures not equal in structure block builder"));
		}
		
	}
	else
	{
		m_curmembers.sinkar.push_back( sink);
	}
}

void StructBlockBuilder::addLastDocStructure( const IndexRange& src)
{
	packCurrentMembers();

	StructureDef st;
	st.header_start = src.start();
	st.header_end = src.end();
	st.membersIdx = m_memberar.size();
	st.membersSize = 0;
	m_structurear.push_back( st);
	++m_structurelistar.back().size;

	m_curmembers.init( m_lastDoc, src);
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
				StructBlockMemberEnum::Iterator itr( m_enumMemberar.data() + stdef.membersIdx, stdef.membersSize);
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
	int estimatedConsumption = (m_curmembers.sinkar.size() + nofstructures) * sizeof(StructureMemberRange);
	return size() + estimatedConsumption <= Constants::maxStructBlockSize();
}

StructBlock StructBlockBuilder::createBlock()
{
	if (empty()) throw strus::runtime_error( _TXT("tried to create empty structure block"));
	packCurrentMembers();

	StructBlock::BlockHeader hdr;

	int docindexofs = sizeof( StructBlock::BlockHeader);
	int structlistofs = docindexofs + m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]);
	int structofs = structlistofs + m_structurelistar.size() * sizeof( m_structurelistar[0]);
	int memberofs = structofs + m_structurear.size() * sizeof( m_structurear[0]);
	int enumofs = memberofs + m_memberar.size() * sizeof( m_memberar[0]);
	int repeatofs = enumofs + m_enumar.size() * sizeof( m_enumar[0]);
	int startofs = repeatofs + m_repeatar.size() * sizeof( m_repeatar[0]);
	int blksize = startofs + m_startar.size() * sizeof( m_startar[0]);

	int first_docno = m_docIndexNodeArray[0].firstDoc();
	int last_docno = m_docIndexNodeArray.back().lastDoc();

	if (m_startar.size() > (std::size_t)StructBlock::StructureMember::MaxMemberIdx)
	{
		throw strus::runtime_error(_TXT("number of start indirection members (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_enumMemberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
	}
	if (m_enumar.size() > (std::size_t)StructBlock::StructureMember::MaxMemberIdx)
	{
		throw strus::runtime_error(_TXT("number of enumeration members (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_enumMemberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
	}
	if (m_repeatar.size() > (std::size_t)StructBlock::StructureMember::MaxMemberIdx)
	{
		throw strus::runtime_error(_TXT("number of repeating structure members (%d) for documents [%d,%d] exceeds maximum limit %d"), (int)m_enumMemberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
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
	hdr.enumidx = enumofs;
	hdr.repeatidx = repeatofs;
	hdr.startidx = startofs;

	MemBlock blkmem( blksize);
	char* dt = (char*)blkmem.ptr();
	std::memcpy( dt, &hdr, sizeof(hdr));
	std::memcpy( dt+docindexofs, m_docIndexNodeArray.data(), m_docIndexNodeArray.size() * sizeof( m_docIndexNodeArray[0]));
	std::memcpy( dt+structlistofs, m_structurelistar.data(), m_structurelistar.size() * sizeof( m_structurelistar[0]));
	std::memcpy( dt+structofs, m_structurear.data(), m_structurear.size() * sizeof( m_structurear[0]));
	std::memcpy( dt+memberofs, m_memberar.data(), m_memberar.size() * sizeof( m_memberar[0]));
	std::memcpy( dt+enumofs, m_enumar.data(), m_enumar.size() * sizeof( m_enumar[0]));
	std::memcpy( dt+startofs, m_startar.data(), m_startar.size() * sizeof( m_startar[0]));

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
	m_startar.clear();
	m_enumar.clear();
	m_repeatar.clear();
	m_curmembers.clear();
	m_lastDoc = 0;
	m_id = 0;
}

