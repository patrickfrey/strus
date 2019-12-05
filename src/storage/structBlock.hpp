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
#include "strus/constants.hpp"
#include "strus/index.hpp"
#include <vector>

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
	typedef unsigned short MemberIdxType;

	struct BlockHeader
	{
		unsigned short structlistidx;
		unsigned short structidx;
		unsigned short memberidx;
		unsigned short _RESERVED;
	};
	struct StructureMember
	{
		PositionType start;
		PositionType end;
	};
	struct StructureRepeat
	{
		enum {ID=0xFFFF};
		PositionType _;/*== ID*/

		unsigned char ofs;
		unsigned char size;
	};
	struct StructureDefList
	{
		unsigned short idx;
		unsigned short size;
	};
	struct StructureDef
	{
		PositionType header_start;
		PositionType header_end;
		MemberIdxType membersSize;
		MemberIdxType membersIdx;
	};
	struct StructureMemberSearchCompare
	{
		StructureMemberSearchCompare(){}
		bool operator()( const StructureMember& aa, const PositionType& bb) const
		{
			return aa.end <= bb;
		}
	};
	struct StructureDefSearchCompare
	{
		StructureDefSearchCompare(){}
		bool operator()( const StructureDef& aa, const PositionType& bb) const
		{
			return aa.header_end <= bb;
		}
	};

public:
	explicit StructBlock()
		:DataBlock(),m_docIndexNodeArray(),m_structlistar(0),m_structar(0),m_memberar(0)
	{}
	StructBlock( const StructBlock& o)
		:DataBlock(o)
	{
		initFrame();
	}
	StructBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
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
	Index lastDoc() const
	{
		return m_docIndexNodeArray.lastDoc();
	}
	bool full() const
	{
		return (int)size() >= Constants::maxStructBlockSize();
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
		bool next()
		{
			if (m_itr < m_ar.size())
			{
				++m_itr;
				return true;
			}
			return false;
		}

		strus::IndexRange skip( const Index& pos)
		{
			if (m_itr >= m_ar.size())
			{
				if (m_itr == 0) return strus::IndexRange();
				m_itr = 0;
			}
			else if ((Index)m_ar[ m_itr].end <= pos)
			{
				int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureMemberSearchCompare());
				if (idx >= 0) m_itr = idx; else return strus::IndexRange();
			}
			return strus::IndexRange( m_ar[ m_itr].start, m_ar[ m_itr].end);
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
		bool next()
		{
			if (m_itr < m_ar.size())
			{
				++m_itr;
				return true;
			}
			return false;
		}

		strus::IndexRange skip( const Index& pos)
		{
			if (m_itr >= m_ar.size())
			{
				if (m_itr == 0) return strus::IndexRange();
				m_itr = 0;
			}
			if ((Index)m_ar[ m_itr].header_end <= pos)
			{
				int idx = m_ar.upperbound( pos, m_itr, m_ar.size(), StructureDefSearchCompare());
				if (idx > 0) m_itr = idx; else return strus::IndexRange();
			}
			return strus::IndexRange( m_ar[ m_itr].header_start, m_ar[ m_itr].header_end);
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


class StructBlockBuilder
{
public:
	typedef StructBlock::StructureMember StructureMember;
	typedef StructBlock::StructureDef StructureDef;
	typedef StructBlock::StructureDefList StructureDefList;

public:
	StructBlockBuilder( const StructBlock& o);
	StructBlockBuilder()
		:m_docIndexNodeArray(),m_memberar(),m_structurelistar(),m_structurear()
		,m_lastDoc(0),m_id(0){}
	StructBlockBuilder( const StructBlockBuilder& o)
		:m_docIndexNodeArray(o.m_docIndexNodeArray)
		,m_memberar(o.m_memberar)
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

		StructDeclaration( const Index& docno_, const IndexRange& src_, const IndexRange& sink_)
			:docno(docno_),src(src_),sink(sink_){}
		StructDeclaration( const StructDeclaration& o)
			:docno(o.docno),src(o.src),sink(o.sink){}
	};

	/// \brief Add a new structure relation to the block
	void append( const Index& docno, const strus::IndexRange& src, const strus::IndexRange& sink);

	/// \brief Add the structures of a document stored in another block 
	void appendFromBlock( const Index& docno, const StructBlock& blk, const DocIndexNodeCursor& cursor);

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
	const std::vector<StructureMember>& memberArray() const			{return m_memberar;}
	const std::vector<StructureDefList>& structureListArray() const		{return m_structurelistar;}
	const std::vector<StructureDef>& structureArray() const			{return m_structurear;}

	StructBlock createBlock() const;
	void clear();

	int size() const
	{
		int dd = m_docIndexNodeArray.size() * sizeof(DocIndexNode);
		int mm = m_memberar.size() * sizeof(StructureMember);
		int ll = m_structurelistar.size() * sizeof(StructureDefList);
		int ss = m_structurear.size() * sizeof(StructureDef);
		return dd + mm + ll + ss;
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
		m_memberar.swap( o.m_memberar);
		m_structurelistar.swap( o.m_structurelistar);
		m_structurear.swap( o.m_structurear);
		std::swap( m_lastDoc, o.m_lastDoc);
		std::swap( m_id, o.m_id);
	}
	
private:
	bool isFittingRepeatMember( const strus::IndexRange& sink);
	void addNewDocument( const Index& docno);
	void addLastDocStructure( const strus::IndexRange& src);
	void addLastStructureMember( const strus::IndexRange& sink);

private:
	struct Cursor
	{
		Cursor() :aridx(0),docidx(0),docno(0),stuidx(0),stuend(0),mbridx(0),mbrend(0){}

		std::size_t aridx;
		unsigned short docidx;
		strus::Index docno;
		std::size_t stuidx;
		std::size_t stuend;
		std::size_t mbridx;
		std::size_t mbrend;
	};

	strus::Index currentMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		if (cursor.mbridx < cursor.mbrend)
		{
			src = strus::IndexRange(
				m_structurear[ cursor.stuidx].header_start,
				m_structurear[ cursor.stuidx].header_end);
			sink = strus::IndexRange(
				m_memberar[ cursor.mbridx].start,
				m_memberar[ cursor.mbridx].end);
			return cursor.docno;
		}
		return 0;
	}

	strus::Index nextMemberNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		++cursor.mbridx;
		return currentMemberNode( cursor, src, sink);
	}

	strus::Index nextStructureFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		for (++cursor.stuidx; cursor.stuidx < cursor.stuend; ++cursor.stuidx)
		{
			cursor.mbridx = m_structurear[ cursor.stuidx].membersIdx;
			cursor.mbrend = cursor.mbridx + m_structurear[ cursor.stuidx].membersSize;
			if (cursor.mbridx < cursor.mbrend) return currentMemberNode( cursor, src, sink);
		}
		return 0;
	}

	strus::Index resolveDocFirstStructure( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		strus::Index ref = m_docIndexNodeArray[ cursor.aridx].ref[ cursor.docidx];
		cursor.stuidx = m_structurelistar[ ref].idx;
		cursor.stuend = cursor.stuidx + m_structurelistar[ ref].size;
		--cursor.stuidx;
		return nextStructureFirstNode( cursor, src, sink);
	}

	strus::Index nextDocFirstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		cursor.docno = m_docIndexNodeArray[ cursor.aridx].nextDoc( cursor.docidx);
		return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
	}

	strus::Index nextIndexFirstDoc( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		for (++cursor.aridx; cursor.aridx < m_docIndexNodeArray.size(); ++cursor.aridx)
		{
			cursor.docno = m_docIndexNodeArray[ cursor.aridx].firstDoc( cursor.docidx);
			return cursor.docno ? resolveDocFirstStructure( cursor, src, sink) : 0;
		}
		return 0;
	}

	strus::Index firstNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		cursor.aridx = -1;
		return nextIndexFirstDoc( cursor, src, sink);
	}

	strus::Index nextNode( Cursor& cursor, strus::IndexRange& src, strus::IndexRange& sink) const
	{
		if (nextMemberNode( cursor, src, sink)) return cursor.docno;
		if (nextStructureFirstNode( cursor, src, sink)) return cursor.docno;
		if (nextDocFirstNode( cursor, src, sink)) return cursor.docno;
		if (nextIndexFirstDoc( cursor, src, sink)) return cursor.docno;
		return 0;
	}

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

