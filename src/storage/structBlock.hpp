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
#include "docIndexNode.hpp"
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
		MaxBlockSize=1024,
		MaxFieldIdxType=0xFFffU
	};
	typedef unsigned short PositionType;
	typedef unsigned short LinkIdxType;
	typedef unsigned short FieldIdxType;
	typedef unsigned short FieldSizeType;

	enum FieldType {
		FieldOffsetType,
		FieldIndexType,
		FieldEnumType,
		FieldRepeatType,
		FieldPackedByteType,
		FieldPackedShortType
	};

	struct BlockHeader
	{
		strus::Index linkidx;
		strus::Index fieldidx;
		strus::Index enumidx;
		strus::Index repeatidx;
		strus::Index startidx;
		strus::Index pkbyteidx;
		strus::Index pkshortidx;
		strus::Index _;
	};
	class StructureLink;
	class StructureField;

	class BlockData
	{
	public:
		BlockData()
			:m_linkar(0),m_fieldar(0),m_enumar(0),m_repeatar(0),m_startar(0){}
		BlockData(
				const StructureLink* linkar_, 
				const StructureField* fieldar_,
				const StructBlockFieldEnum* enumar_,
				const StructBlockFieldRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockFieldPackedByte* pkbytear_,
				const StructBlockFieldPackedShort* pkshortar_)
			:m_linkar(linkar_),m_fieldar(fieldar_)
			,m_enumar(enumar_),m_repeatar(repeatar_),m_startar(startar_)
			,m_pkbytear(pkbytear_),m_pkshortar(pkshortar_){}
		BlockData( const BlockData& o)
			:m_linkar(o.m_linkar),m_fieldar(o.m_fieldar)
			,m_enumar(o.m_enumar),m_repeatar(o.m_repeatar),m_startar(o.m_startar)
			,m_pkbytear(o.m_pkbytear),m_pkshortar(o.m_pkshortar){}

		void init(
				const StructureLink* linkar_, 
				const StructureField* fieldar_,
				const StructBlockFieldEnum* enumar_,
				const StructBlockFieldRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockFieldPackedByte* pkbytear_,
				const StructBlockFieldPackedShort* pkshortar_)
		{
			m_linkar=linkar_; m_fieldar=fieldar_;
			m_enumar=enumar_; m_repeatar=repeatar_; m_startar=startar_;
			m_pkbytear=pkbytear_; m_pkshortar=pkshortar_;
		}
		void init() {init(0,0,0,0,0,0,0);}

		const StructureLink* linkar() const			{return m_linkar;}
		const StructureField* fieldar() const			{return m_fieldar;}
		const StructBlockFieldEnum* enumar() const		{return m_enumar;}
		const StructBlockFieldRepeat* repeatar() const		{return m_repeatar;}
		const PositionType* startar() const			{return m_startar;}
		const StructBlockFieldPackedByte* pkbytear() const	{return m_pkbytear;}
		const StructBlockFieldPackedShort* pkshortar() const	{return m_pkshortar;}

	private:
		const StructureLink* m_linkar;
		const StructureField* m_fieldar;
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

		unsigned short m_type:3;
		unsigned short m_idx:13;
		LinkIdxType m_linkidx;
		LinkIdxType m_linksize;

	public:
		enum {MaxFieldIdx=(1<<13)-1};

		strus::Index end() const	{return m_end;}
		FieldType fieldType() const	{return (FieldType)m_type;}
		FieldIdxType fieldidx() const	{return (FieldIdxType)m_idx;}
		LinkIdxType linkidx() const	{return m_linkidx;}
		LinkIdxType linksize() const	{return m_linksize;}

		void addLink()			{++m_linksize;}

		StructureField( PositionType end_, FieldType type_, FieldIdxType idx_, LinkIdxType linkidx_, LinkIdxType linksize_)
			:m_end(end_),m_type(type_),m_idx(idx_),m_linkidx(linkidx_),m_linksize(linksize_){}
		StructureField( const StructureField& o)
			:m_end(o.m_end),m_type(o.m_type),m_idx(o.m_idx),m_linkidx(o.m_linkidx),m_linksize(o.m_linksize){}

		bool defined() const
		{
			return !!m_end;
		}
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const StructureField& aa, PositionType bb) const
			{
				return aa.m_end <= bb;
			}
		};

		class Iterator
		{
		public:
			Iterator()
				:m_data(0),m_ar(0,0),m_aridx(0),m_cur(){}
			Iterator( const BlockData* data_, const StructureField* ar_, std::size_t arsize_)
				:m_data(data_),m_ar(ar_,arsize_),m_aridx(0),m_cur(){}
			Iterator( const Iterator& o)
				:m_data(o.m_data),m_ar(o.m_ar),m_aridx(o.m_aridx),m_cur(o.m_cur){}
			Iterator& operator=( const Iterator& o)
				{m_data=o.m_data; m_ar=o.m_ar; m_aridx=o.m_aridx; m_cur = o.m_cur; return *this;}

			bool initialized() const
			{
				return m_cur.defined();
			}

			strus::IndexRange next()	{return skip( m_cur.end());}
			strus::IndexRange current()	{return m_cur;}

			strus::IndexRange skip( Index pos);

			void clear()
			{
				m_ar.init( 0, 0);
				m_aridx = 0;
				m_cur = strus::IndexRange();
			}

		private:
			const BlockData* m_data;
			SkipScanArray<StructureField,strus::Index,StructureField::SearchCompare> m_ar;
			int m_aridx;
			strus::IndexRange m_cur;
		};
	};

	struct StructureLink
	{
		StructureLink( strus::Index structno_, bool head_, int idx_)
			:structno(structno_),head(head_),idx(idx_){}
		StructureLink( const StructureLink& o)
			:structno(o.structno),head(o.head),idx(o.idx){}

		unsigned char structno;
		unsigned short head:1;
		unsigned short idx:15;
	};

public:
	explicit StructBlock()
		:DataBlock(),m_docIndexNodeArray(),m_structlistar(0),m_data()
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

	/// \brief Get the document number of the current DocIndexNodeCursor
	Index docno_at( const DocIndexNodeCursor& cursor) const
	{
		return m_docIndexNodeArray.docno_at( cursor);
	}
	/// \brief Get the next document with the current cursor
	Index nextDoc( DocIndexNodeCursor& cursor) const
	{
		return m_docIndexNodeArray.nextDoc( cursor);
	}
	/// \brief Get the first document with the current cursor
	Index firstDoc( DocIndexNodeCursor& cursor) const
	{
		return m_docIndexNodeArray.firstDoc( cursor);
	}
	Index lastDoc() const
	{
		return m_docIndexNodeArray.lastDoc();
	}
	bool full() const
	{
		return (int)size() >= Constants::maxStructBlockSize();
	}
	/// \brief Upper bound search for a docnument number in the block
	Index skipDoc( Index docno_, DocIndexNodeCursor& cursor) const
	{
		return m_docIndexNodeArray.skipDoc( docno_, cursor);
	}

	bool isThisBlockAddress( Index docno_) const
	{
		return (docno_ <= id() && m_docIndexNodeArray.size && docno_ > m_docIndexNodeArray.ar[ 0].base);
	}
	/// \brief Check if the address 'docno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( Index docno_) const
	{
		Index diff = id() - (m_docIndexNodeArray.size?m_docIndexNodeArray.ar[ 0].base:1);
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
	}

	StructureDef::Iterator structureIterator( const DocIndexNodeCursor& cursor) const
	{
		int nofstructs;
		const StructureDef* st = structures_at( cursor, nofstructs);
		if (!st) return StructureDef::Iterator();
		return StructureDef::Iterator( &m_data, st, nofstructs);
	}

	const BlockData* data() const				{return &m_data;}
	const StructureDefList* structlistar() const		{return m_structlistar;}
	const StructureDef* structar() const			{return m_data.structar();}
	const StructureMember* fieldar() const			{return m_data.fieldar();}
	const StructBlockFieldEnum* enumar() const		{return m_data.enumar();}
	const StructBlockFieldRepeat* repeatar() const		{return m_data.repeatar();}
	const PositionType* startar() const			{return m_data.startar();}
	const StructBlockFieldPackedByte* pkbytear() const	{return m_data.pkbytear();}
	const StructBlockFieldPackedShort* pkshortar() const	{return m_data.pkshortar();}

private:
	/// \brief Get the internal representation of the postions of the current DocIndexNodeCursor
	const StructureDef* structures_at( const DocIndexNodeCursor& cursor, int& nofstructs) const
	{
		const StructureDefList* lst = m_structlistar + m_docIndexNodeArray.ref_at( cursor);
		nofstructs = lst->size;
		return m_data.structar() + lst->idx;
	}
	void initFrame();

private:
	DocIndexNodeArray m_docIndexNodeArray;
	const StructureDefList* m_structlistar;
	BlockData m_data;
};


}//namespace
#endif

