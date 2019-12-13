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
#include "structBlockMemberEnum.hpp"
#include "structBlockMemberRepeat.hpp"
#include "docIndexNode.hpp"
#include "skipScanArray.hpp"
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <vector>
#include <cstring>

namespace strus {

/// \class StructBlock
/// \brief Block of structures defined as unidirectional relation between position ranges in a document with the relation source ranges not overlapping.
class StructBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};
	typedef unsigned short PositionType;
	typedef unsigned short StructIdxType;
	typedef unsigned short MemberIdxType;
	typedef unsigned short MemberSizeType;

	enum MemberType {
		StartOffsetType = 0,
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

	struct StructureMember
	{
	private:
		PositionType m_end;

		unsigned short m_type:3;
		unsigned short m_idx:13;

	public:
		enum {MaxMemberIdx=(1<<13)-1};

		strus::Index end() const	{return m_end();}
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
			bool operator()( const StructureMemberRange& aa, const PositionType& bb) const
			{
				return aa.m_end <= bb;
			}
		};

		struct Iterator
		{
			const StructBlock* block;
			SkipScanArray<StructureMember,strus::Index,StructureMember::SearchCompare> ar;
			int aridx;
			strus::IndexRange cur;

			Iterator()
				:block(0),ar(0,0),aridx(0),cur(){}
			Iterator( const StructBlock* block_, const StructureMember* ar_, std::size_t arsize_)
				:block(block_),ar(ar_,arsize_),aridx(0),cur(){}
			Iterator( const Iterator& o)
				:block(o.block),ar(o.ar),aridx(o.aridx),cur(o.cur){}
			Iterator& operator=( const Iterator& o)
				{block=o.block; ar=o.ar; aridx=o.aridx; cur = o.cur; return *this;}

			strus::IndexRange next()	{return skip( cur.end());}
			strus::IndexRange current()	{return cur;}

			strus::IndexRange skip( Index pos);
		};
	};

	struct StructureDefList
	{
		StructIdxType idx;
		StructIdxType size;
	};
	struct StructureDef
	{
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
	};

public:
	explicit StructBlock()
		:DataBlock(),m_docIndexNodeArray(),m_structlistar(0),m_structar(0),m_memberar(0),m_enumMemberar(0)
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
	/// \brief Get the internal representation of the postions of the current DocIndexNodeCursor
	const StructureDef* structures_at( const DocIndexNodeCursor& cursor, int& nofstructs) const
	{
		const StructureDefList* lst = m_structlistar + m_docIndexNodeArray.ref_at( cursor);
		nofstructs = lst->size;
		return m_structar + lst->idx;
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

	class MemberScanner
	{
	public:
		MemberScanner()
			:m_structureType(StructureDef::TypeRangeList)
		{
			m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(0,0);
		}
		MemberScanner( const StructureMemberRange* ar_, int size_)
			:m_structureType(StructureDef::TypeRangeList)
		{
			m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(ar_,size_);
		}
		MemberScanner( const StructBlockMemberEnum* ar_, int size_)
			:m_structureType(StructureDef::TypeEnumList)
		{
			m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(ar_,size_);
		}
		MemberScanner( const MemberScanner& o)
			:m_structureType(o.m_structureType)
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(*o.m_itr.memberIterator);
					break;
				case StructureDef::TypeEnumList:
					m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(*o.m_itr.enumerationIterator);
					break;
			}
		}
		void init( const StructureMemberRange* ar_, int size_)
		{
			m_structureType = StructureDef::TypeRangeList;
			m_itr.memberIterator=new (mem) StructureMemberRange::Iterator(ar_,size_);
		}
		void init( const StructBlockMemberEnum* ar_, int size_)
		{
			m_structureType = StructureDef::TypeEnumList;
			m_itr.enumerationIterator=new (mem) StructBlockMemberEnum::Iterator(ar_,size_);
		}
		IndexRange next()
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					return m_itr.memberIterator->next();
				case StructureDef::TypeEnumList:
					return m_itr.enumerationIterator->next();
			}
		}

		strus::IndexRange skip( Index pos)
		{
			switch (m_structureType)
			{
				case StructureDef::TypeRangeList:
					return m_itr.memberIterator->skip( pos);
				case StructureDef::TypeEnumList:
					return m_itr.enumerationIterator->skip( pos);
			}
			return strus::IndexRange();
		}

	private:
		StructureDef::StructureType m_structureType;
		union
		{
			StructureMemberRange::Iterator* memberIterator;
			StructBlockMemberEnum::Iterator* enumerationIterator;
		} m_itr;
		int mem[ (sizeof(StructureMemberRange::Iterator) + sizeof(StructBlockMemberEnum::Iterator)) / sizeof(int)];
	};

	class StructureScanner
	{
	public:
		StructureScanner()
			:m_ar(0,0),m_itr(0),m_memberar(0){}
		StructureScanner( const StructureDef* ar_, int size_, const StructureMemberRange* memberar_, const StructBlockMemberEnum* enumMemberar_)
			:m_ar(ar_,size_),m_itr(0),m_memberar(memberar_),m_enumMemberar(enumMemberar_){}
		StructureScanner( const StructureScanner& o)
			:m_ar(o.m_ar),m_itr(o.m_itr),m_memberar(o.m_memberar),m_enumMemberar(o.m_enumMemberar){}

		bool initialized() const
		{
			return !!m_ar.size();
		}
		void init( const StructureDef* ar_, int size_)
		{
			m_ar.init( ar_, size_);
		}
		void clear()
		{
			m_ar.init(0,0);
		}
		const StructureDef* current() const
		{
			return m_itr < m_ar.size() ? m_ar.at(m_itr) : 0;
		}
		bool next()
		{
			if (m_itr < m_ar.size())
			{
				++m_itr;
				return true;
			}
			return false;
		}

		strus::IndexRange skip( Index pos)
		{
			if (m_itr >= m_ar.size())
			{
				if (m_itr == 0) return strus::IndexRange();
				m_itr = 0;
			}
			if ((Index)m_ar[ m_itr].header_end <= pos)
			{
				int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureDef::SearchCompare());
				if (idx > 0) m_itr = idx; else return strus::IndexRange();
			}
			return strus::IndexRange( m_ar[ m_itr].header_start, m_ar[ m_itr].header_end);
		}

		MemberScanner members() const
		{
			if (m_itr < m_ar.size())
			{
				const StructureDef& st = m_ar[ m_itr];
				switch (st.structureType)
				{
					case StructureDef::TypeRangeList:
						return MemberScanner( m_memberar + st.membersIdx, st.membersSize);
					case StructureDef::TypeEnumList:
						return MemberScanner( m_enumMemberar + st.membersIdx, st.membersSize);
						break;
				}
				throw std::runtime_error(_TXT("index curruption: bad structure type"));
			}
			else
			{
				return MemberScanner();
			}
		}

	private:
		SkipScanArray<StructureDef,Index,StructureDef::SearchCompare> m_ar;
		int m_itr;
		const StructureMemberRange* m_memberar;
		const StructBlockMemberEnum* m_enumMemberar;
	};

	StructureScanner structureScanner_at( const DocIndexNodeCursor& cursor) const
	{
		int nofstructs;
		const StructureDef* st = structures_at( cursor, nofstructs);
		if (!st) return StructureScanner();
		return StructureScanner( st, nofstructs, m_memberar, m_enumMemberar);
	}

	const StructureDefList* structlistar() const	{return m_structlistar;}
	const StructureDef* structar() const		{return m_structar;}
	const StructureMember* memberar() const		{return m_memberar;}
	const StructBlockMemberEnum* enumar() const	{return m_enumar;}
	const StructBlockMemberRepeat* repeatar() const	{return m_repeatar;}
	const PositionType* startar() const		{return m_startar;}

private:
	void initFrame();

private:
	DocIndexNodeArray m_docIndexNodeArray;
	const StructureDefList* m_structlistar;
	const StructureDef* m_structar;
	const StructureMember* m_memberar;
	const StructBlockMemberEnum* m_enumar;
	const StructBlockMemberRepeat* m_repeatar;
	const PositionType* m_startar;
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
		,m_lastDoc(0),m_id(0){}
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
		,m_id(o.m_id){}

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

	const std::vector<DocIndexNode>& docIndexNodeArray() const		{return m_docIndexNodeArray;}
	const std::vector<StructureDefList>& structureListArray() const		{return m_structurelistar;}
	const std::vector<StructureDef>& structureArray() const			{return m_structurear;}
	const std::vector<StructureMemberRange>& memberArray() const		{return m_memberar;}
	const std::vector<StructBlockMemberEnum>& enumar() const		{return m_enumar;}
	const std::vector<StructBlockMemberRepeat>& repeatar() const		{return m_repeatar;}
	const std::vector<PositionType>& startar() const			{return m_startar;}

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
	}
	
private:
	void addNewDocument( Index docno);
	void addLastDocStructure( const strus::IndexRange& src);
	void addLastStructureMemberRange( const strus::IndexRange& sink);

private:
	struct Cursor
	{
		Cursor()
			:aridx(0),docidx(0),docno(0)
			,stuidx(0),stuend(0),stutype(StructureDef::TypeRangeList)
			,mbridx(0),mbrend(0)
			,repstart(0),repend(0),repofs(0),repsize(0){}

		std::size_t aridx;
		unsigned short docidx;
		strus::Index docno;
		std::size_t stuidx;
		std::size_t stuend;
		StructureDef::StructureType stutype;
		StructBlockMemberEnum::Iterator mitr;
		std::size_t mbridx;
		std::size_t mbrend;
		strus::Index repstart;
		strus::Index repend;
		strus::Index repofs;
		strus::Index repsize;
	};

private:
	struct CurrentMembers
	{
		strus::Index docid;
		strus::IndexRange src;
		std::vector<strus::IndexRange> sinkar;

		CurrentMembers() :docid(0),src(),sinkar(){}
		CurrentMembers( const CurrentMembers& o) :docid(o.docid),src(o.src),sinkar(o.sinkar){}

		void init( strus::Index docid_, const strus::IndexRange& src_)
		{
			docid = docid_;
			src = src_;
			sinkar.clear();
		}
		void clear()
		{
			docid = 0;
			src = strus::IndexRange();
			sinkar.clear();
		}
		void swap( CurrentMembers& o)
		{
			std::swap( docid, o.docid);
			std::swap( src, o.src);
			sinkar.swap( o.sinkar);
		}
	};

	void packCurrentMembers();

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<StructureDefList> m_structurelistar;
	std::vector<StructureDef> m_structurear;
	std::vector<StructureMember> m_memberar;
	std::vector<StructBlockMemberEnum> m_enumar;
	std::vector<StructBlockMemberRepeat> m_repeatar;
	std::vector<PositionType> m_startar;
	
	CurrentMembers m_curmembers;
	Index m_lastDoc;
	Index m_id;
};
}//namespace
#endif

