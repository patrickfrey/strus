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
#include "docIndexNode.hpp"
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <vector>
#include <cstring>
#include <string>

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
		MemberOffsetType = 0,
		MemberIndexType,
		MemberEnumType,
		MemberRepeatType,
	};

	struct BlockHeader
	{
		strus::Index structlistidx;
		strus::Index structidx;
		strus::Index memberidx;
		strus::Index enumidx;
		strus::Index repeatidx;
		strus::Index startidx;
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
				const PositionType* startar_)
			:m_structar(structar_),m_memberar(memberar_),m_enumar(enumar_),m_repeatar(repeatar_),m_startar(startar_){}
		BlockData( const BlockData& o)
			:m_structar(o.m_structar),m_memberar(o.m_memberar),m_enumar(o.m_enumar),m_repeatar(o.m_repeatar),m_startar(o.m_startar){}
		void init(
				const StructureDef* structar_, 
				const StructureMember* memberar_,
				const StructBlockMemberEnum* enumar_,
				const StructBlockMemberRepeat* repeatar_,
				const PositionType* startar_)
			{m_structar=structar_; m_memberar=memberar_; m_enumar=enumar_; m_repeatar=repeatar_; m_startar=startar_;}

		const StructureDef* structar() const		{return m_structar;}
		const StructureMember* memberar() const		{return m_memberar;}
		const StructBlockMemberEnum* enumar() const	{return m_enumar;}
		const StructBlockMemberRepeat* repeatar() const	{return m_repeatar;}
		const PositionType* startar() const		{return m_startar;}

	private:
		const StructureDef* m_structar;
		const StructureMember* m_memberar;
		const StructBlockMemberEnum* m_enumar;
		const StructBlockMemberRepeat* m_repeatar;
		const PositionType* m_startar;
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
			
			strus::IndexRange skip( Index pos)
			{
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

			StructureMember::Iterator memberIterator() const
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

	const BlockData* data() const			{return &m_data;}
	const StructureDefList* structlistar() const	{return m_structlistar;}
	const StructureDef* structar() const		{return m_data.structar();}
	const StructureMember* memberar() const		{return m_data.memberar();}
	const StructBlockMemberEnum* enumar() const	{return m_data.enumar();}
	const StructBlockMemberRepeat* repeatar() const	{return m_data.repeatar();}
	const PositionType* startar() const		{return m_data.startar();}

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

public:
	StructBlockBuilder( const StructBlock& o);
	StructBlockBuilder()
		:m_docIndexNodeArray(),m_structurelistar(),m_structurear(),m_memberar()
		,m_enumar(),m_repeatar(),m_startar(),m_curmembers()
		,m_lastDoc(0),m_id(0),m_membersDropped(0){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_docIndexNodeArray(o.m_docIndexNodeArray)
		,m_structurelistar(o.m_structurelistar)
		,m_structurear(o.m_structurear)
		,m_memberar(o.m_memberar)
		,m_enumar(o.m_enumar)
		,m_repeatar(o.m_repeatar)
		,m_startar(o.m_startar)
		,m_curmembers(o.m_curmembers)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id)
		,m_membersDropped(o.m_membersDropped){}

	Index id() const
	{
		return m_id;
	}
	void setId( Index id_);

	bool empty() const
	{
		return m_structurear.empty();
	}
	Index lastDoc() const
	{
		return m_docIndexNodeArray.empty() ? 0 : m_docIndexNodeArray.back().lastDoc();
	}
	Index firstDoc() const
	{
		return m_docIndexNodeArray.empty() ? 0 : m_docIndexNodeArray[ 0].firstDoc();
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

	int size() const
	{
		int dd = m_docIndexNodeArray.size() * sizeof(m_docIndexNodeArray[0]);
		int ll = m_structurelistar.size() * sizeof(m_structurelistar[0]);
		int ss = m_structurear.size() * sizeof(m_structurear[0]);
		int mm = m_memberar.size() * sizeof(m_memberar[0]);
		int ee = m_enumar.size() * sizeof(m_enumar[0]);
		int ii = m_startar.size() * sizeof(m_startar[0]);
		return dd + ll + ss + mm + ee + ii;
	}

	static void merge( const StructBlockBuilder& blk1, const StructBlockBuilder& blk2, StructBlockBuilder& newblk);

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
	static void split( const StructBlockBuilder& blk, StructBlockBuilder& newblk1, StructBlockBuilder& newblk2);

	void swap( StructBlockBuilder& o)
	{
		m_docIndexNodeArray.swap( o.m_docIndexNodeArray);
		m_structurelistar.swap( o.m_structurelistar);
		m_structurear.swap( o.m_structurear);
		m_memberar.swap( o.m_memberar);
		m_enumar.swap( o.m_enumar);
		m_repeatar.swap( o.m_repeatar);
		m_startar.swap( o.m_startar);
		m_curmembers.swap( o.m_curmembers);
		std::swap( m_lastDoc, o.m_lastDoc);
		std::swap( m_id, o.m_id);
		std::swap( m_membersDropped, o.m_membersDropped);
	}

	int membersDropped() const			{return m_membersDropped;}
	std::string statisticsMessage() const;

private:
	void addNewDocument( Index docno);
	void addLastDocStructure( const strus::IndexRange& src);
	void addLastStructureMemberRange( const strus::IndexRange& sink);

private:
	struct Iterator
	{
		Iterator( const StructBlockBuilder& builder)
			:m_builder(&builder)
			,m_docar( builder.docIndexNodeArray().data(), builder.docIndexNodeArray().size())
			,m_cursor()
			,m_data(
				builder.structurear().data(),
				builder.memberar().data(),
				builder.enumar().data(),
				builder.repeatar().data(),
				builder.startar().data())
			,m_struitr()
			,m_membitr()
			,m_docno(0)
		{}

		strus::Index skipDoc( strus::Index docno)
		{
			m_docno = m_docar.skipDoc( docno, m_cursor);
			m_struitr.clear();
			m_membitr.clear();
			return m_docno;
		}

		bool current( strus::Index& docno_, strus::IndexRange& src_, strus::IndexRange& sink_)
		{
			if (!m_docno) return false;
			src_ = m_struitr.current();
			sink_ = m_membitr.current();
			docno_ = m_docno;
			return true;
		}
		bool next( strus::Index& docno_, strus::IndexRange& src_, strus::IndexRange& sink_)
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
						int ref = m_docar.ref_at( m_cursor);
						int stuidx = m_builder->structurelistar()[ ref].idx;
						int stusize = m_builder->structurelistar()[ ref].size;
						m_struitr = StructureDef::Iterator( &m_data, m_builder->structurear().data() + stuidx, stusize);
						m_membitr = m_struitr.memberIterator();
						src_ = m_struitr.next();
						sink_ = m_membitr.next();
						docno_ = m_docno;
					}
					else
					{
						clear();
						return false;
					}
				}
			}
			return true;
		}

		void clear()
		{
			m_struitr.clear();
			m_membitr.clear();
			m_docno = 0;
		}

		const StructBlockBuilder* m_builder; 
		DocIndexNodeArray m_docar;
		DocIndexNodeCursor m_cursor;
		StructBlock::BlockData m_data;
		StructureDef::Iterator m_struitr;
		StructureMember::Iterator m_membitr;
		strus::Index m_docno;
	};

	Iterator getIterator() const
	{
		return Iterator( *this);
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

		bool defined() const	{return elements&&bytes;}
	};

	MemberDim evaluateMemberDim_offset(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlock::MemberIdxType& ofs);
	MemberDim evaluateMemberDim_index(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			PositionType& start);
	MemberDim evaluateMemberDim_enum(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberEnum& enm);
	MemberDim evaluateMemberDim_repeat(
			std::vector<strus::IndexRange>::const_iterator si,
			std::vector<strus::IndexRange>::const_iterator se,
			StructBlockMemberRepeat& rep);

	static void testPackMember( const StructBlockBuilder::MemberDim& dim, float& maxweight, StructBlock::MemberType& memberType, const StructBlock::MemberType assignMemberType, std::size_t arsize);
	void packCurrentMembers();

	const std::vector<DocIndexNode>& docIndexNodeArray() const	{return m_docIndexNodeArray;}
	const std::vector<StructureDefList>& structurelistar() const	{return m_structurelistar;}
	const std::vector<StructureDef>& structurear() const		{return m_structurear;}
	const std::vector<StructureMember>& memberar() const		{return m_memberar;}
	const std::vector<StructBlockMemberEnum>& enumar() const	{return m_enumar;}
	const std::vector<StructBlockMemberRepeat>& repeatar() const	{return m_repeatar;}
	const std::vector<PositionType>& startar() const		{return m_startar;}

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<StructureDefList> m_structurelistar;
	std::vector<StructureDef> m_structurear;
	std::vector<StructureMember> m_memberar;
	std::vector<StructBlockMemberEnum> m_enumar;
	std::vector<StructBlockMemberRepeat> m_repeatar;
	std::vector<PositionType> m_startar;

	std::vector<strus::IndexRange> m_curmembers;
	Index m_lastDoc;
	Index m_id;
	int m_membersDropped;
};
}//namespace
#endif

