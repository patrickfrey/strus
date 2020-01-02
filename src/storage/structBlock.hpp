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
#include "docIndexNode.hpp"
#include "staticIntrusiveArray.hpp"
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
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
		MaxFieldLevels=8,
		MaxNofStructNo=16,
		MaxNofStructIdx=2048,
		MaxFieldIdx=(1<<13)-1,
		MaxFieldType=7,
		MaxLinkWidth=4
	};
	typedef unsigned short PositionType;
	typedef unsigned short FieldIdx;
	typedef unsigned short PackedLinkBasePointer;

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
		strus::Index fieldidx[ MaxFieldLevels];
		strus::Index linkbaseidx[ MaxFieldLevels];
		strus::Index linkidx[ MaxLinkWidth];
		strus::Index enumidx;
		strus::Index repeatidx;
		strus::Index startidx;
		strus::Index pkbyteidx;
		strus::Index pkshortidx;
		strus::Index _;
	};
	class StructureField;
	typedef StaticIntrusiveArray<StructureField> StructureFieldArray;
	typedef StaticIntrusiveArray<PackedLinkBasePointer> LinkBasePointerArray;
	typedef StaticIntrusiveArray<PackedStructBlockLink> LinkArray;

	class BlockData
	{
	public:
		BlockData()
		{
			init(0,0,0,0,0,0,0,0,0);
		}

		BlockData(
				const StructureFieldArray* fieldar_,
				int fieldarsize_,
				const LinkBasePointerArray* linkbasear_,	//... array parallel to fieldar_
				const LinkArray* linkar_,			//... fixed size array [MaxLinkWidth]
				const StructBlockFieldEnum* enumar_,
				const StructBlockFieldRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockFieldPackedByte* pkbytear_,
				const StructBlockFieldPackedShort* pkshortar_)
		{
			init( fieldar_,fieldarsize_,linkbasear_,linkar_,enumar_,repeatar_,startar_,pkbytear_,pkshortar_);
		}

		BlockData( const BlockData& o)
		{
			std::memcpy( this, &o, sizeof(*this));
		}

		void init()
		{
			init(0,0,0,0,0,0,0,0,0);
		}

		void init(
			const StructureFieldArray* fieldar_,
			int fieldarsize_,
			const LinkBasePointerArray* linkbasear_,	//... array parallel to fieldar_
			const LinkArray* linkar_,			//... fixed size array [MaxLinkWidth]
			const StructBlockFieldEnum* enumar_,
			const StructBlockFieldRepeat* repeatar_,
			const PositionType* startar_,
			const StructBlockFieldPackedByte* pkbytear_,
			const StructBlockFieldPackedShort* pkshortar_);

		const StructureFieldArray& fieldar( int idx) const		{return m_fieldar[ idx];}
		int fieldarsize() const						{return m_fieldarsize;}
		const LinkBasePointerArray& linkbasear( int idx) const		{return m_linkbasear[ idx];}
		const LinkArray& linkar( int idx) const				{return m_linkar[ idx];}
		const StructBlockFieldEnum* enumar() const			{return m_enumar;}
		const StructBlockFieldRepeat* repeatar() const			{return m_repeatar;}
		const PositionType* startar() const				{return m_startar;}
		const StructBlockFieldPackedByte* pkbytear() const		{return m_pkbytear;}
		const StructBlockFieldPackedShort* pkshortar() const		{return m_pkshortar;}

	private:
		StructureFieldArray m_fieldar[ MaxFieldLevels];
		int m_fieldarsize;
		LinkBasePointerArray m_linkbasear[ MaxFieldLevels];
		LinkArray m_linkar[ MaxLinkWidth];
		const StructBlockFieldEnum* m_enumar;
		const StructBlockFieldRepeat* m_repeatar;
		const PositionType* m_startar;
		const StructBlockFieldPackedByte* m_pkbytear;
		const StructBlockFieldPackedShort* m_pkshortar;
	};

	struct StructureField
	{
	private:
		PositionType m_end;

		unsigned short m_type:3;	//... bound to MaxFieldType=7
		unsigned short m_idx:13;	//... bound to MaxFieldIdx=(1<<13)-1

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
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const StructureField& aa, strus::Index bb) const
			{
				return (strus::Index)(unsigned int)aa.m_end <= bb;
			}
		};
	}
#if defined(__GNUC__) || defined(__clang__)
	__attribute__((__packed__))
#endif
	;

	class FieldScanner
	{
	public:
		FieldScanner()
			:m_data(0),m_ar(0,0),m_aridx(0),m_fieldLevel(0),m_cur(),m_linkar(),m_linksize(0){}
		FieldScanner( const BlockData* data_, const StructureFieldArray& fields, int fieldLevel_)
			:m_data(data_),m_ar(fields.ar,fields.size),m_aridx(0),m_fieldLevel(fieldLevel_),m_cur(),m_linkar(),m_linksize(0){}
		FieldScanner( const FieldScanner& o)
			:m_data(o.m_data),m_ar(o.m_ar),m_aridx(o.m_aridx),m_fieldLevel(o.m_fieldLevel),m_cur(o.m_cur),m_linksize(o.m_linksize)
		{
			for (int ii=0; ii<o.m_linksize; ++ii) m_linkar[ii] = o.m_linkar[ii];
		}
		FieldScanner& operator=( const FieldScanner& o)
			{m_data=o.m_data; m_ar=o.m_ar; m_aridx=o.m_aridx; m_fieldLevel=o.m_fieldLevel; m_cur = o.m_cur; m_linksize = o.m_linksize; for (int ii=0; ii<o.m_linksize; ++ii) m_linkar[ii] = o.m_linkar[ii]; return *this;}

		bool initialized() const
		{
			return m_cur.defined();
		}

		strus::IndexRange next()				{return skip( m_cur.end());}
		strus::IndexRange current()				{return m_cur;}

		const StructBlockLink* links() const			{return m_linkar;}
		int noflinks() const					{return m_linksize;}

		strus::IndexRange skip( strus::Index pos);

		void reset()
		{
			m_aridx = 0;
			m_cur = strus::IndexRange();
			m_linksize = 0;
		}

	private:
		const BlockData* m_data;
		SkipScanArray<StructureField,strus::Index,StructureField::SearchCompare> m_ar;
		int m_aridx;
		int m_fieldLevel;
		strus::IndexRange m_cur;
		StructBlockLink m_linkar[ MaxLinkWidth];
		int m_linksize;
	};

	struct LinkBasePointer
	{
		LinkBasePointer()
			:index(0),width(0){}
		LinkBasePointer( const PackedLinkBasePointer& pp)
			{setValue(pp);}
		LinkBasePointer( int index_, int width_)
			:index(index_),width(width_){}
		LinkBasePointer( const LinkBasePointer& o)
			:index(o.index),width(o.width){}

		int index;		//...index of the first link (14 bits)
		int width;		//...number links per element (0->1,2->2,3->4, 2 bits, bound to MaxLinkWidth=4)

		PackedLinkBasePointer value() const
		{
			return ((width & 0x3) << 14) + (index & 0x3fFF);
		}

		void setValue( PackedLinkBasePointer vv)
		{
			width = ((vv & 15) != 0);
			index = vv & 0x3fFF;
		}
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
		const std::vector<StructBlockFieldPackedShort>& pkshortar_);

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

private:
	void initFrame();

private:
	BlockData m_data;
};


}//namespace
#endif

