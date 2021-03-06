/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRUCTURE_BLOCK_HPP_INCLUDED
#define _STRUS_STRUCTURE_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "structBlockFieldRepeat.hpp"
#include "structBlockFieldEnum.hpp"
#include "structBlockFieldPacked.hpp"
#include "structBlockLink.hpp"
#include "structBlockDeclaration.hpp"
#include "docIndexNode.hpp"
#include "staticIntrusiveArray.hpp"
#include "private/skipScanArray.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/storage/index.hpp"
#include "strus/base/stdint.h"
#include "strus/base/packed.h"
#include <vector>
#include <cstring>
#include <string>
#include <map>

namespace strus {

/// \class StructBlock
/// \brief Block of structures defined as unidirectional relation between position ranges in a document with the relation source ranges not overlapping.
class StructBlock
	:public DataBlock
{
public:
	enum {
		NofFieldLevels=8,
		MaxNofStructNo=(1<<7)-1,
		MaxNofStructIdx=(1<<16)-1,
		MaxFieldIdx=(1<<13)-1,
		MaxFieldType=NofFieldLevels-1,
		MaxLinkBaseIdx=(1<<22)-1,
		MaxLinkWidth=(1<<2)-1
	};
	typedef unsigned short PositionType;
	typedef unsigned short FieldIdx;

	enum FieldType {
		FieldTypeOffset,
		FieldTypeIndex,
		FieldTypeEnum,
		FieldTypeRepeat,
		FieldTypePackedByte,
		FieldTypePackedShort
	};

	struct BlockHeader
	{
		strus::Index fieldidx[ NofFieldLevels];
		strus::Index linkbaseidx[ NofFieldLevels];
		strus::Index linkidx[ MaxLinkWidth+1];
		strus::Index enumidx;
		strus::Index repeatidx;
		strus::Index startidx;
		strus::Index pkbyteidx;
		strus::Index pkshortidx;
		strus::Index headeridx;
	};

	PACKED_STRUCT( LinkBasePointer )
	{
		LinkBasePointer( int index_, int width_)
			:index(index_),width(width_){}
		LinkBasePointer( const LinkBasePointer& o)
			:index(o.index),width(o.width){}

		unsigned int index :22;		///< index of the first link
		unsigned int width :2;		///< number links per element (>=1), max bound to MaxLinkWidth=3)

		bool operator == (const LinkBasePointer& o) const
		{
			return index == o.index && width == o.width;
		}
		bool operator < (const LinkBasePointer& o) const
		{
			return width == o.width ? index < o.index : width < o.width;
		}
	};

	PACKED_STRUCT( StructureField )
	{
	private:
		PositionType m_end;

		unsigned short m_type:3;	//... max bound to MaxFieldType=7
		unsigned short m_idx:13;	//... max bound to MaxFieldIdx=(1<<13)-1

	public:
		strus::Index end() const	{return m_end;}
		FieldType fieldType() const	{return (FieldType)m_type;}
		FieldIdx fieldIdx() const	{return (FieldIdx)m_idx;}

		StructureField( PositionType end_, FieldType type_, FieldIdx idx_)
			:m_end(end_),m_type(type_),m_idx(idx_){}
		StructureField( const StructureField& o)
			:m_end(o.m_end),m_type(o.m_type),m_idx(o.m_idx){}

		bool defined() const
		{
			return !!m_end;
		}
		/// \brief Comparator functor to find fields that potentially contain the needle (end > needle)
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const StructureField& aa, strus::Index bb) const
			{
				return (strus::Index)(unsigned int)aa.m_end <= bb;
			}
		};
	};

	typedef StaticIntrusiveArray<StructureField> StructureFieldArray;
	typedef StaticIntrusiveArray<LinkBasePointer> LinkBasePointerArray;
	typedef StaticIntrusiveArray<StructBlockLink> LinkArray;
	typedef StaticIntrusiveArray<PositionType> HeaderStartArray;

	class BlockData
	{
	public:
		BlockData()
		{
			init();
		}

		BlockData(
				const StructureFieldArray* fieldar_,
				int fieldarsize_,
				const LinkBasePointerArray* linkbasear_,	//... array parallel to fieldar_
				const LinkArray* linkar_,			//... fixed size array [MaxLinkWidth+1]
				const StructBlockFieldEnum* enumar_,
				const StructBlockFieldRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockFieldPackedByte* pkbytear_,
				const StructBlockFieldPackedShort* pkshortar_,
				const PositionType* headerar_,
				int headerarsize_)
		{
			init( fieldar_,fieldarsize_,linkbasear_,linkar_,enumar_,repeatar_,startar_,pkbytear_,pkshortar_,headerar_,headerarsize_);
		}

		void init()
		{
			init(0,0,0,0,0,0,0,0,0,0,0);
		}

		void init(
			const StructureFieldArray* fieldar_,
			int fieldarsize_,
			const LinkBasePointerArray* linkbasear_,	//... array parallel to fieldar_
			const LinkArray* linkar_,			//... fixed size array [MaxLinkWidth+1]
			const StructBlockFieldEnum* enumar_,
			const StructBlockFieldRepeat* repeatar_,
			const PositionType* startar_,
			const StructBlockFieldPackedByte* pkbytear_,
			const StructBlockFieldPackedShort* pkshortar_,
			const PositionType* headerar_,
			int headerarsize_);

		const StructureFieldArray& fieldar( int idx) const		{return m_fieldar[ idx];}
		int fieldarsize() const						{return m_fieldarsize;}
		const LinkBasePointerArray& linkbasear( int idx) const		{return m_linkbasear[ idx];}
		const LinkArray& linkar( int idx) const				{return m_linkar[ idx];}
		const StructBlockFieldEnum* enumar() const			{return m_enumar;}
		const StructBlockFieldRepeat* repeatar() const			{return m_repeatar;}
		const PositionType* startar() const				{return m_startar;}
		const StructBlockFieldPackedByte* pkbytear() const		{return m_pkbytear;}
		const StructBlockFieldPackedShort* pkshortar() const		{return m_pkshortar;}
		const HeaderStartArray& headerar() const			{return m_headerar;}

		strus::Index headerStart( int structidx) const
		{
			return (structidx > 0 && structidx <= (int)m_headerar.size) ? m_headerar[structidx-1]:0;
		}

	private:
		StructureFieldArray m_fieldar[ NofFieldLevels];
		int m_fieldarsize;
		LinkBasePointerArray m_linkbasear[ NofFieldLevels];
		LinkArray m_linkar[ MaxLinkWidth+1];
		const StructBlockFieldEnum* m_enumar;
		const StructBlockFieldRepeat* m_repeatar;
		const PositionType* m_startar;
		const StructBlockFieldPackedByte* m_pkbytear;
		const StructBlockFieldPackedShort* m_pkshortar;
		HeaderStartArray m_headerar;
	};

	class FieldScanner
	{
	public:
		FieldScanner()
			:m_data(0),m_ar(0,0),m_aridx(0),m_fieldLevel(0),m_cur(),m_curlnk(-1),m_curwidth(-1){}
		FieldScanner( const BlockData* data_, const StructureFieldArray& fields, int fieldLevel_)
			:m_data(data_),m_ar(fields.ar,fields.size),m_aridx(0),m_fieldLevel(fieldLevel_),m_cur(),m_curlnk(-1),m_curwidth(-1){}
		FieldScanner( const FieldScanner& o)
			:m_data(o.m_data),m_ar(o.m_ar),m_aridx(o.m_aridx),m_fieldLevel(o.m_fieldLevel),m_cur(o.m_cur),m_curlnk(o.m_curlnk),m_curwidth(o.m_curwidth){}
		FieldScanner& operator=( const FieldScanner& o)
			{m_data=o.m_data; m_ar=o.m_ar; m_aridx=o.m_aridx; m_fieldLevel=o.m_fieldLevel; m_cur = o.m_cur; m_curlnk = o.m_curlnk; m_curwidth = o.m_curwidth; return *this;}

		bool initialized() const
		{
			return m_cur.defined();
		}

		strus::IndexRange next()
			{return skip( m_cur.end());}
		strus::IndexRange current() const
			{return m_cur;}
		StructureLinkArray getLinks() const;

		strus::IndexRange skip( strus::Index pos);

		void reset()
		{
			m_aridx = 0;
			m_cur.clear();
			m_curlnk = -1;
			m_curwidth = -1;
		}

	private:
		const BlockData* m_data;
		SkipScanArray<StructureField,strus::Index,StructureField::SearchCompare> m_ar;
		int m_aridx;
		int m_fieldLevel;
		strus::IndexRange m_cur;
		int m_curlnk;
		int m_curwidth;
	};

public:
	explicit StructBlock()
		:DataBlock(),m_data()
	{}
	StructBlock( const StructBlock& o)
		:DataBlock(o)
	{
		initFrame();
	}
	StructBlock( Index id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
	{
		initFrame();
	}

	StructBlock(
		strus::Index docno_,
		const std::vector<std::vector<StructureField> >& fieldar_,
		const std::vector<std::vector<LinkBasePointer> >& linkbasear_,
		const std::vector<std::vector<StructBlockLink> >& linkar_,
		const std::vector<StructBlockFieldEnum>& enumar_,
		const std::vector<StructBlockFieldRepeat>& repeatar_,
		const std::vector<PositionType>& startar_,
		const std::vector<StructBlockFieldPackedByte>& pkbytear_,
		const std::vector<StructBlockFieldPackedShort>& pkshortar_,
		const std::vector<PositionType>& headerar_);

	StructBlock& operator=( const StructBlock& o)
	{
		DataBlock::operator =(o);
		initFrame();
		return *this;
	}
	void swap( DataBlock& o)
	{
		DataBlock::swap( o);
		initFrame();
	}

	const BlockData* data() const					{return &m_data;}
	const StructureFieldArray& fieldar( int idx) const		{return m_data.fieldar( idx);}
	FieldScanner fieldscanner( int levelidx) const			{return FieldScanner( &m_data, m_data.fieldar( levelidx), levelidx);}
	int fieldarsize() const						{return m_data.fieldarsize();}
	const LinkBasePointerArray& linkbasear( int levelidx) const	{return m_data.linkbasear( levelidx);}
	const LinkArray& linkar( int idx) const				{return m_data.linkar( idx);}
	const StructBlockFieldEnum* enumar() const			{return m_data.enumar();}
	const StructBlockFieldRepeat* repeatar() const			{return m_data.repeatar();}
	const PositionType* startar() const				{return m_data.startar();}
	const StructBlockFieldPackedByte* pkbytear() const		{return m_data.pkbytear();}
	const StructBlockFieldPackedShort* pkshortar() const		{return m_data.pkshortar();}
	const HeaderStartArray& headerar() const			{return m_data.headerar();}
	strus::Index headerStart( int structidx) const			{return m_data.headerStart(structidx);}

	std::vector<StructBlockDeclaration> declarations() const;
	std::vector<strus::IndexRange> fields() const;

	void print( std::ostream& out) const;

	void clear()
	{
		DataBlock::clear();
		m_data.init();
	}

private:
	void initFrame();

private:
	BlockData m_data;
};


}//namespace
#endif

