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
#include "docIndexNode.hpp"
#include "skipScanArray.hpp"
#include "strus/index.hpp"
#include <vector>

namespace strus {

/// \class StructureBlock
/// \brief Block of structures defined as unidirectional relation between position ranges in a document with the relation source ranges not overlapping.
class StructureBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};
	typedef Index DocnoType;
	typedef unsigned short PositionType;
	typedef unsigned short MemberIdxType;

	struct BlockHeader
	{
		unsigned short docindexsize;
		unsigned short structlistidx;
		unsigned short structidx;
		unsigned short memberidx;
	};
	struct StructureMember
	{
		PositionType start;
		PositionType end;
	};
	struct StructureDefList
	{
		unsigned short idx;
		unsigned short size;
	};
	struct StructureDef
	{
		PositionType header_end;
		PositionType header_start;
		MemberIdxType membersSize;
		MemberIdxType membersIdx;
	};
	struct StructureMemberSearchCompare
	{
		StructureMemberSearchCompare(){}
		bool operator()( const StructureMember& aa, const PositionType& bb) const
		{
			return aa.end < bb;
		}
	};
	struct StructureDefSearchCompare
	{
		StructureDefSearchCompare(){}
		bool operator()( const StructureDef& aa, const PositionType& bb) const
		{
			return aa.header_end < bb;
		}
	};

public:
	explicit StructureBlock()
		:DataBlock(),m_docIndexNodeArray(),m_structlistar(0),m_structar(0),m_memberar(0)
	{}
	StructureBlock( const StructureBlock& o)
		:DataBlock(o)
		{initFrame();}
	StructureBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
		{initFrame();}

	StructureBlock& operator=( const StructureBlock& o)
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
		const StructureDefList* lst = m_structlistar + m_docIndexNodeArray[ cursor].ref[ cursor.docidx];
		nofstructs = lst->size;
		return m_structar + lst->idx;
	}
	/// \brief Get the members of a structure at a DocIndexNodeCursor position
	const StructureMember* members_at( const DocIndexNodeCursor& cursor, const StructureDef* stdef, int& nofmembers) const
	{
		nofmembers = stdef->membersSize;
		return m_memberar + stdef->membersIdx;
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
	/// \brief Upper bound search for a docnument number in the block
	Index skipDoc( const Index& docno_, DocIndexNodeCursor& cursor) const
	{
		return m_docIndexNodeArray.skipDoc( docno_, cursor);
	}

	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && m_docIndexNodeArray.size && docno_ > m_docIndexNodeArray.ar[ 0].base);
	}
	/// \brief Check if the address 'docno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - (m_docIndexNodeArray.size?m_docIndexNodeArray.ar[ 0].base:1);
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
	}

	class MemberScanner
	{
	public:
		MemberScanner()
			:m_ar(0,0),m_itr(0){}
		MemberScanner( const StructureMember* ar_, int size_)
			:m_ar(ar_,size_),m_itr(0){}
		MemberScanner( const MemberScanner& o)
			:m_ar(o.m_ar),m_itr(o.m_itr){}

		bool initialized() const
		{
			return !!m_ar.size();
		}
		void init( const StructureMember* ar_, int size_)
		{
			m_ar.init( ar_, size_);
		}
		void clear()
		{
			m_ar.init(0,0);
		}
		const StructureMember* current() const
		{
			return m_itr < m_ar.size() ? m_ar.at(m_itr) : 0;
		}

		IndexRange skip( const Index& pos)
		{
			if (initialized()) return IndexRange();
			if ((Index)m_ar[ m_itr].start > pos)
			{
				int idx = m_ar.upperbound( pos, 0, m_itr, StructureMemberSearchCompare());
				if (idx > 0) m_itr = idx; else return IndexRange();
			}
			else if ((Index)m_ar[ m_itr].end <= pos)
			{
				int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureMemberSearchCompare());
				if (idx > 0) m_itr = idx; else return IndexRange();
			}
			return IndexRange( m_ar[ m_itr].start, m_ar[ m_itr].end);
		}
	private:
		SkipScanArray<StructureMember,Index,StructureMemberSearchCompare> m_ar;
		int m_itr;
	};

	class StructureScanner
	{
	public:
		StructureScanner()
			:m_ar(0,0),m_itr(0),m_memberar(0){}
		StructureScanner( const StructureDef* ar_, int size_, const StructureMember* memberar_)
			:m_ar(ar_,size_),m_itr(0),m_memberar(memberar_){}
		StructureScanner( const StructureScanner& o)
			:m_ar(o.m_ar),m_itr(o.m_itr),m_memberar(o.m_memberar){}

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

		IndexRange skip( const Index& pos)
		{
			if (initialized()) return IndexRange();
			if ((Index)m_ar[ m_itr].header_start > pos)
			{
				int idx = m_ar.upperbound( pos, 0, m_itr, StructureDefSearchCompare());
				if (idx > 0) m_itr = idx; else return IndexRange();
			}
			else if ((Index)m_ar[ m_itr].header_end <= pos)
			{
				int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureDefSearchCompare());
				if (idx > 0) m_itr = idx; else return IndexRange();
			}
			return IndexRange( m_ar[ m_itr].header_start, m_ar[ m_itr].header_end);
		}

		MemberScanner members() const
		{
			if (m_itr < m_ar.size())
			{
				const StructureDef& st = m_ar[ m_itr];
				return MemberScanner( m_memberar + st.membersIdx, st.membersSize);
			}
			else
			{
				return MemberScanner( 0, 0);
			}
		}

	private:
		SkipScanArray<StructureDef,Index,StructureDefSearchCompare> m_ar;
		int m_itr;
		const StructureMember* m_memberar;
	};

	StructureScanner structureScanner_at( const DocIndexNodeCursor& cursor) const
	{
		int nofstructs;
		const StructureDef* st = structures_at( cursor, nofstructs);
		if (!st) return StructureScanner();
		return StructureScanner( st, nofstructs, m_memberar);
	}

private:
	void initFrame();

private:
	DocIndexNodeArray m_docIndexNodeArray;
	const StructureDefList* m_structlistar;
	const StructureDef* m_structar;
	const StructureMember* m_memberar;
};


class StructureBlockBuilder
{
public:
	typedef StructureBlock::StructureMember StructureMember;
	typedef StructureBlock::StructureDef StructureDef;
	typedef StructureBlock::StructureDefList StructureDefList;

public:
	StructureBlockBuilder( const StructureBlock& o);
	StructureBlockBuilder()
		:m_memberar(),m_structurelistar(),m_structurear(),m_lastDoc(0),m_id(0){}
	StructureBlockBuilder( const StructureBlockBuilder& o)
		:m_memberar(o.m_memberar)
		,m_structurelistar(o.m_structurelistar)
		,m_structurear(o.m_structurear)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}

	Index id() const
	{
		return m_id;
	}
	void setId( const Index& id_);

	bool empty() const
	{
		return m_structurear.empty();
	}

	void push( const Index& docno, const IndexRange& src, const IndexRange& sink);

	bool fitsInto( std::size_t nofstructures) const;
	bool full() const
	{
		int dd = m_docIndexNodeArray.size() * sizeof(DocIndexNode);
		int mm = m_memberar.size() * sizeof(StructureMember);
		int ll = m_structurelistar.size() * sizeof(StructureDefList);
		int ss = m_structurear.size() * sizeof(StructureDef);
		return dd + mm + ll + ss >= StructureBlock::MaxBlockSize;
	}

	const std::vector<DocIndexNode>& docIndexNodeArray() const		{return m_docIndexNodeArray;}
	const std::vector<StructureMember>& memberArray() const			{return m_memberar;}
	const std::vector<StructureDefList>& structureListArray() const		{return m_structurelistar;}
	const std::vector<StructureDef>& structureArray() const			{return m_structurear;}

	StructureBlock createBlock() const;
	void clear();

private:
	void addNewDocument( const Index& docno);
	void addLastDocStructure( const IndexRange& src);
	void addLastStructureMember( const IndexRange& sink);

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<StructureMember> m_memberar;
	std::vector<StructureDefList> m_structurelistar;
	std::vector<StructureDef> m_structurear;
	Index m_lastDoc;
	Index m_id;
};
}//namespace
#endif

