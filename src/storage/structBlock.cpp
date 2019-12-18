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
		m_data.init( 0, 0, 0, 0, 0);
	}	
	else if (size() >= sizeof(BlockHeader))
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		int blockSize = (int)DataBlock::size();

		const DocIndexNode* docIndexNodePtr = getStructPtr<DocIndexNode>( "document index nodes", ptr(), sizeof(BlockHeader), hdr->structlistidx);
		int docIndexNodeSize = getStructSize<DocIndexNode>( "document index nodes", sizeof(BlockHeader), hdr->structlistidx);
		m_docIndexNodeArray.init( docIndexNodePtr, docIndexNodeSize);
		m_structlistar = getStructPtr<StructureDefList>( "structure definition list", ptr(), hdr->structlistidx, hdr->structidx);
		const StructureDef* structar_ = getStructPtr<StructureDef>( "structure definitions", ptr(), hdr->structidx, hdr->memberidx);
		const StructureMember* memberar_ = getStructPtr<StructureMember>( "member definitions", ptr(), hdr->memberidx, hdr->enumidx);
		const StructBlockMemberEnum* enumar_ = getStructPtr<StructBlockMemberEnum>( "enum definitions", ptr(), hdr->enumidx, hdr->repeatidx);
		const StructBlockMemberRepeat* repeatar_ = getStructPtr<StructBlockMemberRepeat>( "enumeration member definitions", ptr(), hdr->repeatidx, hdr->startidx);
		const PositionType* startar_ = getStructPtr<PositionType>( "start definitions", ptr(), hdr->startidx, blockSize);
		m_data.init( structar_, memberar_, enumar_, repeatar_, startar_);
	}
	else
	{
		throw strus::runtime_error( _TXT("data corruption in structure block: %s"), _TXT("header not defined defined"));
	}
}

strus::IndexRange StructBlock::StructureMember::Iterator::skip( strus::Index pos)
{
	if (pos < m_cur.end() && pos >= m_cur.start())
	{
		return m_cur;
	}
	if (m_aridx >= m_ar.size())
	{
		if (m_aridx == 0) return m_cur = strus::IndexRange();
		m_aridx = 0;
	}
	if (m_ar[ m_aridx].end() <= pos)
	{
		int idx = m_ar.upperbound( pos, m_aridx+1, m_ar.size(), StructureMember::SearchCompare());
		if (idx >= 0) m_aridx = idx; else return m_cur = strus::IndexRange();
	}
	else
	{
		int idx = m_ar.upperbound( pos, 0, m_aridx+1, StructureMember::SearchCompare());
		if (idx >= 0) m_aridx = idx; else return m_cur = strus::IndexRange();
	}
	const StructureMember& ths = m_ar[ m_aridx];
	switch (ths.memberType())
	{
		case MemberOffsetType:
			m_cur = strus::IndexRange( ths.end() - ths.memberIdx(), ths.end());
			break;
		case MemberIndexType:
			m_cur = strus::IndexRange( m_data->startar()[ ths.memberIdx()], ths.end());
			break;
		case MemberEnumType:
			m_cur = m_data->enumar()[ ths.memberIdx()].skip( pos);
			break;
		case MemberRepeatType:
			m_cur = m_data->repeatar()[ ths.memberIdx()].skip( pos);
			if (m_cur.end() > m_ar[ m_aridx].end()) m_cur = strus::IndexRange();
			break;
	}
	return m_cur;
}

strus::IndexRange StructBlock::StructureDef::Iterator::skip( Index pos)
{
	if (m_cur.end() > pos && m_cur.start() <= pos)
	{
		return m_cur;
	}
	if (m_aridx >= m_ar.size())
	{
		if (m_aridx == 0) return m_cur=strus::IndexRange();
		m_aridx = 0;
	}
	if ((Index)m_ar[ m_aridx].header_end <= pos)
	{
		int idx = m_ar.upperbound( pos, m_aridx, m_ar.size(), StructureDef::SearchCompare());
		if (idx > 0) m_aridx = idx; else return m_cur=strus::IndexRange();
		return m_cur=strus::IndexRange( m_ar[ m_aridx].header_start, m_ar[ m_aridx].header_end);
	}
	else if ((Index)m_ar[ m_aridx].header_start <= pos)
	{
		return m_cur=strus::IndexRange( m_ar[ m_aridx].header_start, m_ar[ m_aridx].header_end);
	}
	else
	{
		m_aridx = m_ar.upperbound( pos, 0, m_aridx+1, StructureDef::SearchCompare());
		return m_cur=strus::IndexRange( m_ar[ m_aridx].header_start, m_ar[ m_aridx].header_end);
	}
}

StructBlock::StructureMember::Iterator StructBlock::StructureDef::Iterator::memberIterator() const
{
	if (m_aridx < m_ar.size())
	{
		const StructureDef& st = m_ar[ m_aridx];
		return StructureMember::Iterator( m_data, m_data->memberar() + st.membersIdx, st.membersSize);
	}
	else
	{
		return StructureMember::Iterator();
	}
}


StructBlockBuilder::StructBlockBuilder( const StructBlock& o)
	:m_docIndexNodeArray(),m_structurelistar(),m_structurear(),m_memberar()
	,m_enumar(),m_repeatar(),m_startar(),m_curmembers()
	,m_lastDoc(0),m_id(0),m_membersDropped(0)
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

StructBlockBuilder::MemberDim StructBlockBuilder::evaluateMemberDim_offset(
		std::vector<strus::IndexRange>::const_iterator si,
		std::vector<strus::IndexRange>::const_iterator se,
		StructBlock::MemberIdxType& ofs)
{
	MemberDim rt;
	if (si->end() - si->start() >= StructureMember::MaxMemberIdx) return rt;
	rt.fill = (float)m_memberar.size() / StructBlock::MaxMemberIdxType;
	rt.bytes = sizeof(StructureMember);
	rt.elements = 1;
	rt.end = si->end();
	ofs = si->end() - si->start();
	return rt;
}

StructBlockBuilder::MemberDim StructBlockBuilder::evaluateMemberDim_index(
		std::vector<strus::IndexRange>::const_iterator si,
		std::vector<strus::IndexRange>::const_iterator se,
		PositionType& start)
{
	MemberDim rt;
	rt.fill = ((float)m_memberar.size() / StructBlock::MaxMemberIdxType
			+ m_startar.size() / StructureMember::MaxMemberIdx
		) / 2;
	rt.bytes = sizeof(StructureMember) + sizeof(PositionType);
	rt.elements = 1;
	rt.end = si->end();
	start = si->start();
	return rt;
}

StructBlockBuilder::MemberDim StructBlockBuilder::evaluateMemberDim_enum(
		std::vector<strus::IndexRange>::const_iterator si,
		std::vector<strus::IndexRange>::const_iterator se,
		StructBlockMemberEnum& enm)
{
	MemberDim rt;
	int nn = 0;
	for (; si != se; ++si,++nn)
	{
		if (!enm.append( *si)) break;
		rt.end = si->end();
	}
	if (nn)
	{
		rt.fill = ((float)m_memberar.size() / StructBlock::MaxMemberIdxType
				+ m_enumar.size() / StructureMember::MaxMemberIdx
			) / 2;
		rt.bytes = sizeof(StructureMember) + sizeof(StructBlockMemberEnum);
		rt.elements = nn;
	}
	return rt;
}

StructBlockBuilder::MemberDim StructBlockBuilder::evaluateMemberDim_repeat(
		std::vector<strus::IndexRange>::const_iterator si,
		std::vector<strus::IndexRange>::const_iterator se,
		StructBlockMemberRepeat& rep)
{
	MemberDim rt;
	if (si + 1 < se)
	{
		std::vector<strus::IndexRange>::const_iterator sn = si+1;
		strus::Index end = sn->start();
		int nn = 0;
		for (; si != se && rep.append( end, *si); ++si,++nn){}
		if (nn > 1)
		{
			rt.fill = ((float)m_memberar.size() / StructBlock::MaxMemberIdxType
					+ m_repeatar.size() / StructureMember::MaxMemberIdx
				) / 2;
			rt.bytes = sizeof(StructureMember) + sizeof(StructBlockMemberRepeat);
			rt.elements = nn;
			rt.end = rep.lastMemberEnd( end);
		}
	}
	return rt;
}

void StructBlockBuilder::testPackMember( const StructBlockBuilder::MemberDim& dim, float& maxweight, StructBlock::MemberType& memberType, const StructBlock::MemberType assignMemberType, std::size_t arsize)
{
	if (arsize < StructBlock::StructureMember::MaxMemberIdx)
	{
		float weight = ((float)dim.elements / (float)dim.bytes) * (1.0 - dim.fill);
		if (weight >= maxweight)
		{
			memberType = assignMemberType;
			maxweight = weight;
		}
	}
}

void StructBlockBuilder::packCurrentMembers()
{
	if (m_curmembers.empty()) return;

	std::vector<strus::IndexRange>::const_iterator
		si = m_curmembers.begin(), se = m_curmembers.end();
	while (si != se)
	{
		if (m_memberar.size() < StructBlock::MaxMemberIdxType)
		{
			StructBlock::MemberType memberType = StructBlock::MemberOffsetType;
			float maxweight = 0.0;

			StructBlock::MemberIdxType ofs;
			MemberDim dim_offset = evaluateMemberDim_offset( si, se, ofs);
			testPackMember( dim_offset, maxweight, memberType,
					StructBlock::MemberOffsetType, 0);

			PositionType start;
			MemberDim dim_index = evaluateMemberDim_index( si, se, start);
			testPackMember( dim_index, maxweight, memberType,
					StructBlock::MemberIndexType, m_startar.size());

			StructBlockMemberEnum enm;
			MemberDim dim_enum = evaluateMemberDim_enum( si, se, enm);
			testPackMember( dim_enum, maxweight, memberType,
					StructBlock::MemberEnumType, m_enumar.size());

			StructBlockMemberRepeat rep;
			MemberDim dim_repeat = evaluateMemberDim_repeat( si, se, rep);
			testPackMember( dim_repeat, maxweight, memberType,
					StructBlock::MemberRepeatType, m_repeatar.size());

			if (maxweight > 0.0)
			{
				switch (memberType)
				{
					case StructBlock::MemberOffsetType:
						m_memberar.push_back( StructureMember( dim_offset.end, memberType, ofs));
						si += dim_offset.elements;
						break;
					case StructBlock::MemberIndexType:
						m_memberar.push_back( StructureMember( dim_index.end, memberType, m_startar.size()));
						m_startar.push_back( start);
						si += dim_index.elements;
						break;
					case StructBlock::MemberEnumType:
						m_memberar.push_back( StructureMember( dim_enum.end, memberType, m_enumar.size()));
						m_enumar.push_back( enm);
						si += dim_enum.elements;
						break;
					case StructBlock::MemberRepeatType:
						m_memberar.push_back( StructureMember( dim_repeat.end, memberType, m_repeatar.size()));
						m_repeatar.push_back( rep);
						si += dim_repeat.elements;
						break;
				}
			}
			else
			{
				++m_membersDropped;
				++si;
			}
		}
		else
		{
			++m_membersDropped;
			++si;
		}
	}
	m_structurear.back().membersSize = m_memberar.size() - m_structurear.back().membersIdx;
	m_curmembers.clear();
}

void StructBlockBuilder::addNewDocument( Index docno)
{
	packCurrentMembers();
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

void StructBlockBuilder::addLastStructureMemberRange( const IndexRange& sink)
{
	if (!m_curmembers.empty() && m_curmembers.back().end() >= sink.start())
	{
		if (m_curmembers.back().end() == sink.start())
		{
			m_curmembers.back() = strus::IndexRange( m_curmembers.back().start(), sink.end());
		}
		else if (m_curmembers.back() != sink)
		{
			throw strus::runtime_error( _TXT("overlaping sinks of structures not equal in structure block builder"));
		}
		
	}
	else
	{
		m_curmembers.push_back( sink);
	}
}

void StructBlockBuilder::addLastDocStructure( const IndexRange& src)
{
	packCurrentMembers();

	StructureDef st( src.start(), src.end(), m_memberar.size(), 0);
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
		addLastDocStructure( src);
		addLastStructureMemberRange( sink);
	}
	else if (m_structurelistar.back().size)
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

void StructBlockBuilder::merge( StructBlockBuilder& blk1, StructBlockBuilder& blk2, StructBlockBuilder& newblk)
{
	blk1.packCurrentMembers();
	blk2.packCurrentMembers();
	newblk.packCurrentMembers();

	StructBlockBuilder::Iterator itr1 = blk1.getIterator();	
	strus::Index docno1 = itr1.skipDoc( 0/*docno*/);
	strus::IndexRange src1;
	strus::IndexRange sink1;
	(void)itr1.scanNext( docno1, src1, sink1);

	StructBlockBuilder::Iterator itr2 = blk2.getIterator();
	strus::Index docno2 = itr2.skipDoc( 0/*docno*/);
	strus::IndexRange src2;
	strus::IndexRange sink2;
	(void)itr2.scanNext( docno2, src2, sink2);

	while (docno1 && docno2)
	{
		if (docno1 < docno2)
		{
			strus::Index dn = docno1;
			for ( bool more=true; more && docno1 == dn; more=itr1.scanNext( docno1, src1, sink1))
			{
				newblk.append( docno1, src1, sink1);
			}
		}
		else if (docno1 > docno2)
		{
			strus::Index dn = docno2;

			for ( bool more=true; more && docno2 == dn; more=itr2.scanNext( docno2, src2, sink2))
			{
				newblk.append( docno2, src2, sink2);
			}
		}
		else/*(docno1 == docno2)*/
		{
			throw std::runtime_error(_TXT("cannot merge structure blocks with overlapping document references"));
		}
	}
	while (docno1)
	{
		for ( bool more=true; more; more=itr1.scanNext( docno1, src1, sink1))
		{
			newblk.append( docno1, src1, sink1);
		}
	}
	while (docno2)
	{
		for (bool more=true; more; more=itr2.scanNext( docno2, src2, sink2))
		{
			newblk.append( docno2, src2, sink2);
		}
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
	StructureDef::Iterator sitr = blk.structureIterator( cursor);
	strus::IndexRange source = sitr.next();
	for (; source.defined(); source = sitr.next())
	{
		StructureMember::Iterator mitr = sitr.memberIterator();
		for (strus::IndexRange sink=mitr.next(); sink.defined(); sink=mitr.next())
		{
			append( docno, source, sink);
		}
	}
	packCurrentMembers();
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
	appendblk.packCurrentMembers();

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

void StructBlockBuilder::split( StructBlockBuilder& blk, StructBlockBuilder& newblk1, StructBlockBuilder& newblk2)
{
	blk.packCurrentMembers();

	std::vector<StructDeclaration> declar;
	strus::IndexRange src;
	strus::IndexRange sink;
	StructBlockBuilder::Iterator itr = blk.getIterator();
	strus::Index docno = itr.skipDoc( 0/*docno*/);
	bool more = itr.scanNext( docno, src, sink);
	for (; more; more = itr.scanNext( docno, src, sink))
	{
		declar.push_back( StructDeclaration( docno, src, sink));
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
	int estimatedConsumption = (m_curmembers.size() + nofstructures) * sizeof(StructureMember);
	return size() + estimatedConsumption <= Constants::maxStructBlockSize();
}

StructBlock StructBlockBuilder::createBlock()
{
	if (empty())
	{
		throw strus::runtime_error( _TXT("tried to create empty structure block"));
	}
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
		throw strus::runtime_error(_TXT("number of start indirection members (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_startar.size(), first_docno, last_docno, (int)StructBlock::StructureMember::MaxMemberIdx);
	}
	if (m_enumar.size() > (std::size_t)StructBlock::StructureMember::MaxMemberIdx)
	{
		throw strus::runtime_error(_TXT("number of enumeration members (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_enumar.size(), first_docno, last_docno, (int)StructBlock::StructureMember::MaxMemberIdx);
		std::vector<StructBlockMemberEnum>::const_iterator ei = m_enumar.begin(), ee = m_enumar.end();
		for (; ei != ee; ++ei)
		{
			if (!ei->check()) throw strus::runtime_error(_TXT("corrupt block (enumeration members)"));
		}
	}
	if (m_repeatar.size() > (std::size_t)StructBlock::StructureMember::MaxMemberIdx)
	{
		throw strus::runtime_error(_TXT("number of repeating structure members (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_repeatar.size(), first_docno, last_docno, (int)StructBlock::StructureMember::MaxMemberIdx);
		std::vector<StructBlockMemberRepeat>::const_iterator ri = m_repeatar.begin(), re = m_repeatar.end();
		for (; ri != re; ++ri)
		{
			if (!ri->check()) throw strus::runtime_error(_TXT("corrupt block (enumeration members)"));
		}
	}
	if (m_memberar.size() > (std::size_t)std::numeric_limits<StructBlock::MemberIdxType>::max())
	{
		throw strus::runtime_error(_TXT("number of members (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_memberar.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::MemberIdxType>::max());
		std::vector<StructureMember>::const_iterator mi = m_memberar.begin(), me = m_memberar.end();
		for (; mi != me; ++mi)
		{
			switch (mi->memberType())
			{
				case StructBlock::MemberOffsetType:
					break;
				case StructBlock::MemberIndexType:
					if (mi->memberIdx() >= m_startar.size())
					{
						throw strus::runtime_error(_TXT("corrupt block (start indirections)"));
					}
					break;
				case StructBlock::MemberEnumType:
					if (mi->memberIdx() >= m_enumar.size())
					{
						throw strus::runtime_error(_TXT("corrupt block (enumeration indirections)"));
					}
					break;
				case StructBlock::MemberRepeatType:
					if (mi->memberIdx() >= m_repeatar.size())
					{
						throw strus::runtime_error(_TXT("corrupt block (repeat indirections)"));
					}
					break;
			}
		}
	}
	if (m_structurear.size() > (std::size_t)std::numeric_limits<StructBlock::StructIdxType>::max())
	{
		throw strus::runtime_error(_TXT("number of structures (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_structurear.size(), first_docno, last_docno, (int)std::numeric_limits<StructBlock::StructIdxType>::max());
	}
	if (m_structurelistar.size() > (std::size_t)std::numeric_limits<unsigned short>::max()/*element of DocIndexNode::ref*/)
	{
		throw strus::runtime_error(_TXT("number of structure lists (%d) for documents [%d,%d] exceeds maximum limit %d"),
			(int)m_structurelistar.size(), first_docno, last_docno, (int)std::numeric_limits<unsigned short>::max());
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
	std::memcpy( dt+repeatofs, m_repeatar.data(), m_repeatar.size() * sizeof( m_repeatar[0]));
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
	m_membersDropped = 0;
}

std::string StructBlockBuilder::statisticsMessage() const
{
	return strus::string_format(
		_TXT(
		"dropped members: %d, "
		"number of documents: %d, "
		"number of structures: %d, "
		"number of members: %d, "
		"number of enums: %d, "
		"number of repeats: %d, "
		"number of indirections: %d"
		),
		m_membersDropped,
		(int)m_structurelistar.size(),
		(int)m_structurear.size(),
		(int)m_memberar.size(),
		(int)m_enumar.size(),
		(int)m_repeatar.size(),
		(int)m_startar.size());
}

strus::Index StructBlockBuilder::Iterator::skipDoc( strus::Index docno)
{
	m_docno = m_docar.skipDoc( docno, m_cursor);
	initNodeIterators();
	return m_docno;
}

bool StructBlockBuilder::Iterator::scanNext( strus::Index& docno_, strus::IndexRange& src_, strus::IndexRange& sink_)
{
	sink_ = m_membitr.next();
	if (sink_.defined())
	{
		src_ = m_struitr.current();
		docno_ = m_docno;
	}
	else
	{
		src_ = m_struitr.next();
		if (src_.defined())
		{
			m_membitr = m_struitr.memberIterator();
			sink_ = m_membitr.next();
			docno_ = m_docno;
		}
		else
		{
			m_docno = skipDoc( m_docno+1);
			if (m_docno)
			{
				initNodeIterators();
				src_ = m_struitr.current();
				sink_ = m_membitr.next();
				docno_ = m_docno;
			}
			else
			{
				clear();
				docno_ = 0;
				return false;
			}
		}
	}
	return true;
}

void StructBlockBuilder::Iterator::clear()
{
	m_struitr.clear();
	m_membitr.clear();
	m_docno = 0;
}

void StructBlockBuilder::Iterator::initNodeIterators()
{
	int ref = m_docar.ref_at( m_cursor);
	int stuidx = m_builder->structurelistar()[ ref].idx;
	int stusize = m_builder->structurelistar()[ ref].size;
	m_struitr = StructureDef::Iterator( &m_data, m_builder->structurear().data() + stuidx, stusize);
	m_struitr.next();
	m_membitr = m_struitr.memberIterator();
}



