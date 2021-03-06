/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlock.hpp"
#include "structBlockBuilder.hpp"
#include "memBlock.hpp"
#include "indexPacker.hpp"
#include "strus/lib/structs.hpp"
#include "strus/base/static_assert.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <limits>
#include <set>

using namespace strus;

namespace {
struct _AlignmentBaseStruct {char _;};
#define AlignmentBase sizeof(_AlignmentBaseStruct)

struct StaticAsserts
{
	StaticAsserts()
	{
		STRUS_STATIC_ASSERT( sizeof(StructBlock::StructureField) == 4);
		STRUS_STATIC_ASSERT( sizeof(StructBlockLink) == 3);
		STRUS_STATIC_ASSERT( sizeof(StructBlock::LinkBasePointer) == 3);
		STRUS_STATIC_ASSERT( sizeof(StructBlock::PositionType) == 2);
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

void StructBlock::BlockData::init(
		const StructureFieldArray* fieldar_,
		int fieldarsize_,
		const LinkBasePointerArray* linkbasear_,
		const LinkArray* linkar_,
		const StructBlockFieldEnum* enumar_,
		const StructBlockFieldRepeat* repeatar_,
		const PositionType* startar_,
		const StructBlockFieldPackedByte* pkbytear_,
		const StructBlockFieldPackedShort* pkshortar_,
		const PositionType* headerar_,
		int headerarsize_)
{
	if (fieldarsize_ > NofFieldLevels)
	{
		throw strus::runtime_error(_TXT("number (%d) of levels of overlapping fields exceeds maximum size (%d) allowed"), (int)fieldarsize_, (int)NofFieldLevels);
	}
	m_fieldarsize = fieldarsize_;
	int ii;
	if (fieldar_) for (ii=0; ii != m_fieldarsize; ++ii) m_fieldar[ii] = fieldar_[ii];
	if (linkbasear_) for (ii=0; ii != m_fieldarsize; ++ii) m_linkbasear[ii] = linkbasear_[ii];
	if (linkar_) for (ii=0; ii != MaxLinkWidth+1; ++ii) m_linkar[ii] = linkar_[ii];
	m_enumar = enumar_; 
	m_repeatar = repeatar_;
	m_startar = startar_;
	m_pkbytear = pkbytear_;
	m_pkshortar = pkshortar_;
	m_headerar.init( headerar_, headerarsize_);
}

StructBlock::StructBlock(
	strus::Index docno_,
	const std::vector<std::vector<StructureField> >& fieldar_,
	const std::vector<std::vector<LinkBasePointer> >& linkbasear_,
	const std::vector<std::vector<StructBlockLink> >& linkar_,
	const std::vector<StructBlockFieldEnum>& enumar_,
	const std::vector<StructBlockFieldRepeat>& repeatar_,
	const std::vector<PositionType>& startar_,
	const std::vector<StructBlockFieldPackedByte>& pkbytear_,
	const std::vector<StructBlockFieldPackedShort>& pkshortar_,
	const std::vector<PositionType>& headerar_)
{
	BlockHeader hdr;
	std::memset( &hdr, 0, sizeof( hdr));
	if (fieldar_.size() > NofFieldLevels) throw strus::runtime_error(_TXT("too many levels of fields defined (%d > maximum %d)"), (int)fieldar_.size(), (int)NofFieldLevels);
	if (linkbasear_.size() != fieldar_.size()) throw std::runtime_error(_TXT("link base array size is not equal to parallel field array"));
	if (linkar_.size() > MaxLinkWidth+1) throw strus::runtime_error(_TXT("too many links defined defined per field (%d > maximum %d)"), (int)linkar_.size(), (int)MaxLinkWidth+1);

	int pi = sizeof(hdr);
	{
		int fi = 0, fe = fieldar_.size();
		for (; fi != fe; ++fi)
		{
			pi += fieldar_[fi].size() * sizeof(StructureField);
			hdr.fieldidx[ fi] = pi;
		}
		for (; fi != NofFieldLevels; ++fi)
		{
			hdr.fieldidx[ fi] = 0;
		}
	}{
		int fi = 0, fe = linkbasear_.size();
		for (; fi != fe; ++fi)
		{
			if (linkbasear_[fi].size() != fieldar_[fi].size())
			{
				throw strus::runtime_error(_TXT("link base array [%d] size is not equal to parallel field array"), fi);
			}
			pi += linkbasear_[fi].size() * sizeof(LinkBasePointer);
			hdr.linkbaseidx[ fi] = pi;
		}
		for (; fi != NofFieldLevels; ++fi)
		{
			hdr.linkbaseidx[ fi] = 0;
		}
	}{
		int fi = 0, fe = linkar_.size();
		for (; fi != fe; ++fi)
		{
			pi += linkar_[fi].size() * sizeof(StructBlockLink);
			hdr.linkidx[ fi] = pi;
		}
		for (; fi != MaxLinkWidth+1; ++fi)
		{
			hdr.linkidx[ fi] = 0;
		}
	}
	while (pi % AlignmentBase)
	{
		pi += 1;
	}
	hdr.enumidx = pi;
	pi += enumar_.size() * sizeof( StructBlockFieldEnum);
	hdr.repeatidx = pi;
	pi += repeatar_.size() * sizeof( StructBlockFieldRepeat);
	hdr.pkbyteidx = pi;
	pi += pkbytear_.size() * sizeof( StructBlockFieldPackedByte);
	hdr.pkshortidx = pi;
	pi += pkshortar_.size() * sizeof( StructBlockFieldPackedShort);
	hdr.startidx = pi;
	pi += startar_.size() * sizeof( PositionType);
	hdr.headeridx = pi;
	pi += headerar_.size() * sizeof(PositionType);
	int blockSize = pi;

	DataBlock blk( docno_, blockSize);
	char* dt = blk.charptr();

	int startidx = sizeof(hdr);
	std::memcpy( dt, &hdr, sizeof(hdr));
	{
		int fi = 0, fe = NofFieldLevels;
		for (; fi != fe; ++fi)
		{
			int endidx = hdr.fieldidx[ fi];
			if (endidx > startidx)
			{
				std::memcpy( dt + startidx, fieldar_[fi].data(), endidx-startidx);
				startidx = endidx;
			}
		}
	}{
		int fi = 0, fe = NofFieldLevels;
		for (; fi != fe; ++fi)
		{
			int endidx = hdr.linkbaseidx[ fi];
			if (endidx > startidx)
			{
				std::memcpy( dt + startidx, linkbasear_[fi].data(), endidx-startidx);
				startidx = endidx;
			}
		}
	}{
		int fi = 0, fe = MaxLinkWidth+1;
		for (; fi != fe; ++fi)
		{
			int endidx = hdr.linkidx[ fi];
			if (endidx > startidx)
			{
				std::memcpy( dt + startidx, linkar_[fi].data(), endidx-startidx);
				startidx = endidx;
			}
		}
	}
	std::memcpy( dt + hdr.enumidx, enumar_.data(), hdr.repeatidx - hdr.enumidx);
	std::memcpy( dt + hdr.repeatidx, repeatar_.data(), hdr.pkbyteidx - hdr.repeatidx);
	std::memcpy( dt + hdr.pkbyteidx, pkbytear_.data(), hdr.pkshortidx - hdr.pkbyteidx);
	std::memcpy( dt + hdr.pkshortidx, pkshortar_.data(), hdr.startidx - hdr.pkshortidx);
	std::memcpy( dt + hdr.startidx, startar_.data(), hdr.headeridx - hdr.startidx);
	std::memcpy( dt + hdr.headeridx, headerar_.data(), blockSize - hdr.headeridx);

	this->swap( blk);
}

void StructBlock::initFrame()
{
	if (empty())
	{
		m_data.init();
	}
	else if (size() >= sizeof(BlockHeader))
	{
		const BlockHeader* hdr = (const BlockHeader*)ptr();
		int blockSize = (int)DataBlock::size();

		StructureFieldArray fieldar_[ NofFieldLevels];
		int fieldarsize_ = 0;
		LinkBasePointerArray linkbasear_[ NofFieldLevels];
		LinkArray linkar_[ MaxLinkWidth+1];

		int pi = sizeof(BlockHeader);
		{
			int fi = 0, fe = NofFieldLevels;
			for (; fi != fe && hdr->fieldidx[fi]; ++fi)
			{
				const StructureField* ar = getStructPtr<StructureField>( "field definitions", ptr(), pi, hdr->fieldidx[fi]);
				std::size_t arsize = getStructSize<StructureField>( "field definitions", pi, hdr->fieldidx[fi]);
				fieldar_[ fi].init( ar, arsize);
				pi = hdr->fieldidx[fi];
			}
			fieldarsize_ = fi;
			for (; fi != fe; ++fi)
			{
				fieldar_[ fi].init( 0, 0);
			}
		}{
			int fi = 0, fe = NofFieldLevels;
			fi = 0;
			for (; fi != fe && hdr->linkbaseidx[fi]; ++fi)
			{
				const LinkBasePointer* ar = getStructPtr<LinkBasePointer>( "link base pointer definitions", ptr(), pi, hdr->linkbaseidx[fi]);
				std::size_t arsize = getStructSize<LinkBasePointer>( "link base pointer definitions", pi, hdr->linkbaseidx[fi]);
				linkbasear_[ fi].init( ar, arsize);
				if (arsize != fieldar_[ fi].size) throw std::runtime_error(_TXT("link base pointer array not parallel to structure field array"));
				pi = hdr->linkbaseidx[fi];
			}
			for (; fi != fe; ++fi)
			{
				linkbasear_[ fi].init( 0, 0);
			}
		}{
			int fi = 0, fe = MaxLinkWidth+1;
			for (; fi != fe && hdr->linkidx[ fi]; ++fi)
			{
				const StructBlockLink* ar = getStructPtr<StructBlockLink>( "link definitions", ptr(), pi, hdr->linkidx[fi]);
				std::size_t arsize = getStructSize<StructBlockLink>( "link definitions", pi, hdr->linkidx[fi]);
				linkar_[ fi].init( ar, arsize);
				pi = hdr->linkidx[fi];
			}
			for (; fi != fe; ++fi)
			{
				linkar_[ fi].init( 0, 0);
			}
		}
		const StructBlockFieldEnum* enumar_ = getStructPtr<StructBlockFieldEnum>( "enumeration definitions", ptr(), hdr->enumidx, hdr->repeatidx);
		const StructBlockFieldRepeat* repeatar_ = getStructPtr<StructBlockFieldRepeat>( "repeating field definitions", ptr(), hdr->repeatidx, hdr->pkbyteidx);
		const StructBlockFieldPackedByte* pkbytear_ = getStructPtr<StructBlockFieldPackedByte>( "packed (byte) field definitions", ptr(), hdr->pkbyteidx, hdr->pkshortidx);
		const StructBlockFieldPackedShort* pkshortar_ = getStructPtr<StructBlockFieldPackedShort>( "packed (short) field definitions", ptr(), hdr->pkshortidx, hdr->startidx);
		const PositionType* startar_ = getStructPtr<PositionType>( "big field start definitions", ptr(), hdr->startidx, hdr->headeridx);
		const PositionType* headerar_ = getStructPtr<PositionType>( "map structure index to header field start", ptr(), hdr->headeridx, blockSize);
		int headerarsize_ = getStructSize<PositionType>( "map structure index to header field start", hdr->headeridx, blockSize);

		m_data.init( fieldar_, fieldarsize_, linkbasear_, linkar_, enumar_, repeatar_, startar_, pkbytear_, pkshortar_, headerar_, headerarsize_);
	}
	else
	{
		throw strus::runtime_error( _TXT("data corruption in structure block: %s"), _TXT("block header size does not match"));
	}
}

strus::IndexRange StructBlock::FieldScanner::skip( strus::Index pos)
{
	if (m_cur.contain( pos))
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
		int idx = m_ar.upperbound( pos, m_aridx+1, m_ar.size());
		if (idx >= 0) m_aridx = idx; else return m_cur = strus::IndexRange();
	}
	else if (m_aridx == 0 || m_ar[ m_aridx-1].end() <= pos)
	{
		//... index m_aridx stays as it is
	}
	else
	{
		int idx = m_ar.upperbound( pos, 0, m_aridx);
		if (idx >= 0) m_aridx = idx; else return m_cur = strus::IndexRange();
	}
	const StructureField& ths = m_ar[ m_aridx];
	std::pair<strus::IndexRange,int> loc;

	switch (ths.fieldType())
	{
		case FieldTypeOffset:
			loc.first = strus::IndexRange( ths.end() - ths.fieldIdx(), ths.end());
			loc.second = 0;
			break;
		case FieldTypeIndex:
			loc.first = strus::IndexRange( m_data->startar()[ ths.fieldIdx()], ths.end());
			loc.second = 0;
			break;
		case FieldTypeEnum:
			loc = m_data->enumar()[ ths.fieldIdx()].skip( pos);
			break;
		case FieldTypeRepeat:
			loc = m_data->repeatar()[ ths.fieldIdx()].skip( pos, ths.end());
			break;
		case FieldTypePackedByte:
			loc = m_data->pkbytear()[ ths.fieldIdx()].skip( pos);
			break;
		case FieldTypePackedShort:
			loc = m_data->pkshortar()[ ths.fieldIdx()].skip( pos);
			break;
	}
	m_cur = loc.first;
	LinkBasePointer linkbase( m_data->linkbasear( m_fieldLevel)[ m_aridx]);
	m_curlnk = ((int)linkbase.index + (int)loc.second) * (int)(linkbase.width+1);
	m_curwidth = linkbase.width;
	return m_cur;
}

StructureLinkArray StructBlock::FieldScanner::getLinks() const
{
	StructureLinkArray rt;
	if (m_curlnk >= 0)
	{
		if (m_curwidth > MaxLinkWidth) throw std::runtime_error(_TXT("corrupt data structure: width in field scanner"));
		int li = m_curlnk, le = m_curlnk + m_curwidth+1;
		for (; li < le; li++)
		{
			rt.add( m_data->linkar( m_curwidth)[ li].unpacked());
		}
	}
	return rt;
}

std::vector<strus::IndexRange> StructBlock::fields() const
{
	std::vector<strus::IndexRange> rt;
	int fi = 0, fe = fieldarsize();
	for (; fi != fe; ++fi)
	{
		StructBlock::FieldScanner scanner = fieldscanner( fi);
		strus::IndexRange field = scanner.next();
		for (; field.defined(); field = scanner.next())
		{
			rt.push_back( field);
		}
	}
	return rt;
}

std::vector<StructBlockDeclaration> StructBlock::declarations() const
{
	std::vector<StructBlockDeclaration> rt;
	StorageStructMap::Map declmap;

	int fi = 0, fe = fieldarsize();
	for (; fi != fe; ++fi)
	{
		StructBlock::FieldScanner scanner = fieldscanner( fi);
		strus::IndexRange field = scanner.next();
		for (; field.defined(); field = scanner.next())
		{
			StructureLinkArray links = scanner.getLinks();
			int li = 0, le = links.nofLinks();
			for (; li != le; ++li)
			{
				const StructureLink& link = links[ li];
				if (link.structno() <= 0)
				{
					throw std::runtime_error(_TXT("corrupt index: illegal structure number"));
				}
				StorageStructMap::Key key( link.structno(), link.index());
				std::pair<StorageStructMap::Map::iterator,bool>
					ins = declmap.insert( StorageStructMap::Map::value_type( key, StorageStructMap::FieldRelation()));
				StorageStructMap::Map::iterator di = ins.first;

				if (link.header())
				{
					if (di->second.first.defined())
					{
						throw strus::runtime_error(_TXT("corrupt index: overlapping duplicate structure key: [%d,%d] and [%d,%d]"),
									 (int)di->second.first.start(), (int)di->second.first.end(),
									 (int)field.start(), (int)field.end());
					}
					else
					{
						di->second.first = field;
					}
				}
				else
				{
					di->second.second.push_back( field);
				}
			}
		}
	}
	StorageStructMap::Map::const_iterator mi = declmap.begin(), me = declmap.end();
	for (; mi != me; ++mi)
	{
		strus::IndexRange src = mi->second.first;
		std::vector<strus::IndexRange>::const_iterator
			di = mi->second.second.begin(), de = mi->second.second.end();
		for (; di != de; ++di)
		{
			strus::IndexRange sink = *di;
			rt.push_back( StructBlockDeclaration( mi->first.structno(), src, sink));
		}
	}
	return rt;
}

void StructBlock::print( std::ostream& out) const
{
	out << "docno " << id() << ":" << std::endl;

	std::vector<StructBlockDeclaration> decl = declarations();
	std::vector<StructBlockDeclaration>::const_iterator di = decl.begin(), de = decl.end();
	for (; di != de; ++di)
	{
		out << strus::string_format( "%d [%d,%d] -> [%d,%d]",
					(int)di->structno,
					(int)di->src.start(), (int)di->src.end(),
					(int)di->sink.start(), (int)di->sink.end()) << std::endl;
	}
}


