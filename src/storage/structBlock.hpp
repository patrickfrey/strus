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
#include "structBlockMemberRepeat.hpp"
#include "structBlockMemberEnum.hpp"
#include "structBlockMemberPacked.hpp"
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
		MaxMemberIdxType=0xFFffU
	};
	typedef unsigned short PositionType;
	typedef unsigned short StructIdxType;
	typedef unsigned short MemberIdxType;
	typedef unsigned short MemberSizeType;

	enum MemberType {
		MemberOffsetType,
		MemberIndexType,
		MemberEnumType,
		MemberRepeatType,
		MemberPackedByteType,
		MemberPackedShortType
	};

	struct BlockHeader
	{
		strus::Index structlistidx;
		strus::Index structidx;
		strus::Index memberidx;
		strus::Index enumidx;
		strus::Index repeatidx;
		strus::Index startidx;
		strus::Index pkbyteidx;
		strus::Index pkshortidx;
	};
	class StructureDef;
	class StructureMember;

	class BlockData
	{
	public:
		BlockData()
			:m_structar(0),m_memberar(0),m_enumar(0),m_repeatar(0),m_startar(0){}
		BlockData(
				const StructureDef* structar_, 
				const StructureMember* memberar_,
				const StructBlockMemberEnum* enumar_,
				const StructBlockMemberRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockMemberPackedByte* pkbytear_,
				const StructBlockMemberPackedShort* pkshortar_)
			:m_structar(structar_),m_memberar(memberar_)
			,m_enumar(enumar_),m_repeatar(repeatar_),m_startar(startar_)
			,m_pkbytear(pkbytear_),m_pkshortar(pkshortar_){}
		BlockData( const BlockData& o)
			:m_structar(o.m_structar),m_memberar(o.m_memberar)
			,m_enumar(o.m_enumar),m_repeatar(o.m_repeatar),m_startar(o.m_startar)
			,m_pkbytear(o.m_pkbytear),m_pkshortar(o.m_pkshortar){}

		void init(
				const StructureDef* structar_, 
				const StructureMember* memberar_,
				const StructBlockMemberEnum* enumar_,
				const StructBlockMemberRepeat* repeatar_,
				const PositionType* startar_,
				const StructBlockMemberPackedByte* pkbytear_,
				const StructBlockMemberPackedShort* pkshortar_)
		{
			m_structar=structar_; m_memberar=memberar_;
			m_enumar=enumar_; m_repeatar=repeatar_; m_startar=startar_;
			m_pkbytear=pkbytear_; m_pkshortar=pkshortar_;
		}
		void init() {init(0,0,0,0,0,0,0);}

		const StructureDef* structar() const			{return m_structar;}
		const StructureMember* memberar() const			{return m_memberar;}
		const StructBlockMemberEnum* enumar() const		{return m_enumar;}
		const StructBlockMemberRepeat* repeatar() const		{return m_repeatar;}
		const PositionType* startar() const			{return m_startar;}
		const StructBlockMemberPackedByte* pkbytear() const	{return m_pkbytear;}
		const StructBlockMemberPackedShort* pkshortar() const	{return m_pkshortar;}

	private:
		const StructureDef* m_structar;
		const StructureMember* m_memberar;
		const StructBlockMemberEnum* m_enumar;
		const StructBlockMemberRepeat* m_repeatar;
		const PositionType* m_startar;
		const StructBlockMemberPackedByte* m_pkbytear;
		const StructBlockMemberPackedShort* m_pkshortar;
	};

	struct StructureMember
	{
	private:
		PositionType m_end;

		unsigned short m_type:3;
		unsigned short m_idx:13;

	public:
		enum {MaxMemberIdx=(1<<13)-1};

		strus::Index end() const	{return m_end;}
		MemberType memberType() const	{return (MemberType)m_type;}
		MemberIdxType memberIdx() const	{return (MemberIdxType)m_idx;}

		StructureMember( PositionType end_, MemberType type_, MemberIdxType idx_)
			:m_end(end_),m_type(type_),m_idx(idx_){}
		StructureMember( const StructureMember& o)
			:m_end(o.m_end),m_type(o.m_type),m_idx(o.m_idx){}

		bool defined() const
		{
			return !!m_end;
		}
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const StructureMember& aa, const PositionType& bb) const
			{
				return aa.m_end <= bb;
			}
		};

		class Iterator
		{
		public:
			Iterator()
				:m_data(0),m_ar(0,0),m_aridx(0),m_cur(){}
			Iterator( const BlockData* data_, const StructureMember* ar_, std::size_t arsize_)
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
			SkipScanArray<StructureMember,strus::Index,StructureMember::SearchCompare> m_ar;
			int m_aridx;
			strus::IndexRange m_cur;
		};
	};

	struct StructureDefList
	{
		StructIdxType idx;
		StructIdxType size;
	};

	struct StructureDef
	{
		StructureDef( PositionType header_start_, PositionType header_end_, MemberIdxType membersIdx_, MemberSizeType membersSize_)
			:header_start(header_start_),header_end(header_end_),membersIdx(membersIdx_),membersSize(membersSize_){}
		StructureDef( const StructureDef& o)
			:header_start(o.header_start),header_end(o.header_end),membersIdx(o.membersIdx),membersSize(o.membersSize){}

		PositionType header_start;
		PositionType header_end;
		MemberIdxType membersIdx;
		MemberSizeType membersSize;

		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const StructureDef& aa, const PositionType& bb) const
			{
				return aa.header_end <= bb;
			}
		};

		class Iterator
		{
		public:
			Iterator()
				:m_data(0),m_ar(0,0),m_aridx(0),m_cur(){}
			Iterator( const StructBlock* block_, const StructureDef* ar_, int size_)
				:m_data(block_->data()),m_ar(ar_,size_),m_aridx(0),m_cur(){}
			Iterator( const BlockData* data_, const StructureDef* ar_, int size_)
				:m_data(data_),m_ar(ar_,size_),m_aridx(0),m_cur(){}
			Iterator( const Iterator& o)
				:m_data(o.m_data),m_ar(o.m_ar),m_aridx(o.m_aridx),m_cur(){}

			bool initialized() const
			{
				return !!m_ar.size();
			}
			void init( const StructureDef* ar_, int size_)
			{
				m_ar.init( ar_, size_);
				m_cur = strus::IndexRange();
			}
			void clear()
			{
				m_ar.init(0,0);
				m_aridx = 0;
				m_cur = strus::IndexRange();
			}
			strus::IndexRange next()	{return m_cur=skip( m_cur.end());}
			strus::IndexRange current()	{return m_cur;}
			
			strus::IndexRange skip( Index pos);
			StructureMember::Iterator memberIterator() const;

		private:
			const BlockData* m_data;
			SkipScanArray<StructureDef,Index,StructureDef::SearchCompare> m_ar;
			int m_aridx;
			strus::IndexRange m_cur;
		};
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
	const StructureMember* memberar() const			{return m_data.memberar();}
	const StructBlockMemberEnum* enumar() const		{return m_data.enumar();}
	const StructBlockMemberRepeat* repeatar() const		{return m_data.repeatar();}
	const PositionType* startar() const			{return m_data.startar();}
	const StructBlockMemberPackedByte* pkbytear() const	{return m_data.pkbytear();}
	const StructBlockMemberPackedShort* pkshortar() const	{return m_data.pkshortar();}

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


class StructBlockBuilder
{
public:
	typedef StructBlock::PositionType PositionType;
	typedef StructBlock::StructureMember StructureMember;
	typedef StructBlock::StructureDef StructureDef;
	typedef StructBlock::StructureDefList StructureDefList;
	typedef StructBlock::MemberType MemberType;

public:
	StructBlockBuilder( const StructBlock& o);
	StructBlockBuilder()
		:m_ar(),m_id(0){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_ar(o.m_ar)
		,m_id(o.m_id){}

	Index id() const
	{
		return m_id;
	}
	void setId( Index id_);

	bool empty() const
	{
		return m_ar.empty();
	}
	Index lastDoc() const
	{
		return m_ar.empty() ? 0 : m_ar.back().docno;
	}
	Index firstDoc() const
	{
		return m_ar.empty() ? 0 : m_ar[ 0].docno;
	}

	struct StructDeclaration
	{
		Index docno;
		IndexRange src;
		IndexRange sink;

		StructDeclaration( Index docno_, const IndexRange& src_, const IndexRange& sink_)
			:docno(docno_),src(src_),sink(sink_){}
		StructDeclaration( const StructDeclaration& o)
			:docno(o.docno),src(o.src),sink(o.sink){}
	};

	/// \brief Add a new structure relation to the block
	void append( Index docno, const strus::IndexRange& src, const strus::IndexRange& sink);

	/// \brief Add the structures of a document stored in another block 
	void appendFromBlock( Index docno, const StructBlock& blk, const DocIndexNodeCursor& cursor);

	bool fitsInto( std::size_t nofstructures) const;
	bool full() const
	{
		return size() >= Constants::maxStructBlockSize();
	}
	/// \brief Eval if the block is filled with a given ratio
	/// \param[in] ratio value between 0.0 and 1.0, reasonable is a value close to one
	/// \note A small value leads to fragmentation, a value close to 1.0 leads to transactions slowing down
	bool filledWithRatio( float ratio) const
	{
		return size() >= (int)(ratio * Constants::maxStructBlockSize());
	}

	StructBlock createBlock();
	void clear();

	int size() const;

	static void merge(
			StructBlockBuilder& blk1,
			StructBlockBuilder& blk2,
			StructBlockBuilder& newblk);

	static void merge(
			std::vector<StructDeclaration>::const_iterator ei,
			const std::vector<StructDeclaration>::const_iterator& ee,
			const StructBlock& oldblk,
			StructBlockBuilder& newblk);
	static void merge_append(
			std::vector<StructDeclaration>::const_iterator ei,
			const std::vector<StructDeclaration>::const_iterator& ee,
			const StructBlock& oldblk,
			StructBlockBuilder& appendblk);
	static void split( StructBlockBuilder& blk, StructBlockBuilder& newblk1, StructBlockBuilder& newblk2);

	void swap( StructBlockBuilder& o)
	{
		m_ar.swap( o.m_ar);
		std::swap( m_id, o.m_id);
	}

	int membersDropped() const			{return m_membersDropped;}
	std::string statisticsMessage() const;

private:
	void addNewDocument( Index docno);
	void addLastDocStructure( const strus::IndexRange& src);
	void addLastStructureMemberRange( const strus::IndexRange& sink);

private:
	struct DocStructureMap
	{
		strus::Index docno;
		std::map<strus::IndexRange,int> map;

		DocStructureMap( strus::Index docno_, const std::map<strus::IndexRange,int>& map_)
			:docno(docno_),map(map_){}
		DocStructureMap( const DocStructureMap& o)
			:docno(o.docno),map(o.map){}

		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const DocStructureMap& aa, const strus::Index& docno) const
			{
				return aa.docno <= docno;
			}
		};
	};

	struct DocStructureMapIterator
	{
		DocStructureMapIterator( const DocStructureMapIterator& o)
			:m_ar(o.m_ar)
			,m_aridx(o.m_aridx)
			,m_itr(o.m_itr)
			,m_itr_defined(o.m_itr_defined)
		{}
		DocStructureMapIterator( const StructBlockBuilder& builder)
			:m_ar(&builder.m_ar.data(),builder.m_ar.size())
			,m_aridx(0)
			,m_itr()
			,m_itr_defined(false)
		{}

		strus::Index skipDoc( strus::Index docno)
		{
			if (m_aridx >= (int)m_ar.size()) return 0;
			int idx = 0;
			if (docno <= m_ar[ m_aridx].docno)
			{
				if (docno == m_ar[ m_aridx].docno)
				{
					if (!m_itr_defined)
					{
						m_itr_defined = true;
						m_itr = m_ar[ m_aridx].map.begin();
					}
					return docno;
				}
				idx = m_ar.upperbound( docno, 0, m_aridx+1);
			}
			else
			{
				idx = m_ar.upperbound( docno, m_aridx+1, m_ar.size());
			}
			if (true==(m_itr_defined = (idx >= 0)))
			{
				m_aridx = idx;
				m_itr = m_ar[ m_aridx].map.begin();
				return m_ar[ m_aridx].docno;
			}
			return 0;
		}

		bool scanNext( strus::Index& docno_, strus::IndexRange& range_, int& relid)
		{
			if (m_itr_defined)
			{
				range_ = m_itr->first;
				relid_ = m_itr->second;
				docno_ = m_ar[ m_aridx].docno;
				if (++m_itr == m_ar[ m_aridx].map.end())
				{
					do
					{
						if (m_aridx+1 == m_ar.size())
						{
							m_itr_defined = false;
							return false;
						}
						++m_aridx;
						m_itr = m_ar[ m_aridx].map.begin();
					}
					while (m_itr == m_ar[ m_aridx].map.end());
				}
				return true;
			}
			else
			{
				return false;
			}
		}

		void clear()
		{
			m_itr_defined = false;
			m_aridx = 0;
		}

		SkipScanArray<DocStructureMap,strus::Index,DocStructureMap::SearchCompare> m_ar;
		int m_aridx;
		std::map<strus::IndexRange,int>::const_iterator m_itr;
		bool m_itr_defined;
	};

	DocStructureMapIterator getDocStructureMapIterator() const
	{
		return DocStructureMapIterator( *this);
	}

private:
	struct MemberDim
	{
		int elements;
		int bytes;
		float fill;
		PositionType end;

		MemberDim()			:elements(0),bytes(0),fill(0),end(0){}
		MemberDim( const MemberDim& o)	:elements(o.elements),bytes(o.bytes),fill(o.fill),end(o.end){}

		float weight() const	{return ((float)elements / (float)bytes) * (1.0 - fill);}
		bool defined() const	{return elements&&bytes;}
	};

	MemberDim evaluateMemberDim_offset(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlock::MemberIdxType& ofs) const;
	MemberDim evaluateMemberDim_index(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			PositionType& start) const;
	MemberDim evaluateMemberDim_enum(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberEnum& enm) const;
	MemberDim evaluateMemberDim_repeat(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberRepeat& rep) const;
	MemberDim evaluateMemberDim_pkbyte(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberPackedByte& pkb) const;
	MemberDim evaluateMemberDim_pkshort(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberPackedShort& pks) const;

	MemberType getNextPackMemberType(
		std::vector<strus::IndexRange>::const_iterator si,
		std::vector<strus::IndexRange>::const_iterator se) const;

private:
	std::vector<DocStructureMap> m_ar;
	strus::Index m_id;
};
}//namespace
#endif

