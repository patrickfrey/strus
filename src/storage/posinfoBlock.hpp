/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_POSINFO_BLOCK_HPP_INCLUDED
#define _STRUS_POSINFO_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "docIndexNode.hpp"
#include "strus/constants.hpp"
#include <vector>
#include <utility>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	typedef unsigned short PositionType;

public:
	explicit PosinfoBlock()
		:DataBlock(),m_docIndexNodeArray(),m_posinfoptr(0)
	{}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o)
		{initFrame();}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
		{initFrame();}

	PosinfoBlock& operator=( const PosinfoBlock& o)
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
	const PositionType* posinfo_at( const DocIndexNodeCursor& cursor) const
	{
		return m_posinfoptr + m_docIndexNodeArray[ cursor].ref[ cursor.docidx];
	}
	/// \brief Get the list of the postions of the current DocIndexNodeCursor
	std::vector<Index> positions_at( const DocIndexNodeCursor& cursor) const;

	/// \brief Get the feature frequency of the current DocIndexNodeCursor
	unsigned int frequency_at( const DocIndexNodeCursor& cursor) const;

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

	class PositionScanner
	{
	public:
		PositionScanner()
			:m_ar(0),m_size(0),m_itr(0){}
		PositionScanner( const PositionType* ar_)
			:m_ar(ar_+1),m_size(ar_[0]),m_itr(0){}
		PositionScanner( const PositionScanner& o)
			:m_ar(o.m_ar),m_size(o.m_size),m_itr(o.m_itr){}

		bool initialized() const		{return m_size;}

		void init( const PositionType* ar_)
		{
			if (ar_)
			{
				m_ar = ar_+1;
				m_size = ar_[0];
				m_itr = 0;
			}
			else
			{
				m_ar = 0;
				m_size = 0;
				m_itr = 0;
			}
		}

		void clear()						{init(0);}

		Index curpos() const					{return (m_itr<m_size)?m_ar[m_itr]:0;}
		Index skip( const Index& pos);

	private:
		const PositionType* m_ar;
		unsigned short m_size;
		unsigned short m_itr;
	};

	PositionScanner positionScanner_at( const DocIndexNodeCursor& cursor) const
	{
		return PositionScanner( posinfo_at( cursor));
	}

private:
	void initFrame();

private:
	DocIndexNodeArray m_docIndexNodeArray;
	const PositionType* m_posinfoptr;
};

class PosinfoBlockBuilder
{
public:
	typedef PosinfoBlock::PositionType PositionType;

public:
	PosinfoBlockBuilder( const PosinfoBlock& o);
	PosinfoBlockBuilder()
		:m_lastDoc(0),m_id(0){}
	PosinfoBlockBuilder( const PosinfoBlockBuilder& o)
		:m_docIndexNodeArray(o.m_docIndexNodeArray)
		,m_posinfoArray(o.m_posinfoArray)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}
	PosinfoBlockBuilder& operator=( const PosinfoBlockBuilder& o)
	{
		m_docIndexNodeArray = o.m_docIndexNodeArray;
		m_posinfoArray = o.m_posinfoArray;
		m_lastDoc = o.m_lastDoc;
		m_id = o.m_id;
		return *this;
	}
#if __cplusplus >= 201103L
	PosinfoBlockBuilder& operator=( PosinfoBlockBuilder&& o)
	{
		m_docIndexNodeArray = std::move(o.m_docIndexNodeArray);
		m_posinfoArray = std::move(o.m_posinfoArray);
		m_lastDoc = o.m_lastDoc;
		m_id = o.m_id;
		return *this;
	}
	PosinfoBlockBuilder( PosinfoBlockBuilder&& o)
		:m_docIndexNodeArray(std::move(o.m_docIndexNodeArray))
		,m_posinfoArray(std::move(o.m_posinfoArray))
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}
#endif
	Index id() const						{return m_id;}
	void setId( const Index& id_);

	bool empty() const						{return m_docIndexNodeArray.empty();}

	/// \brief Append document position info
	/// \param[in] docno document number
	/// \param[in] posar pointer to posinfo encoded as: posar[0]=length, posar[1..]=posinfo array
	void append( const Index& docno, const PositionType* posar);

	bool fitsInto( std::size_t nofpos) const;

	bool full() const
	{
		return size() >= Constants::maxPosInfoBlockSize();
	}
	/// \brief Eval if the block is filled with a given ratio
	/// \param[in] ratio value between 0.0 and 1.0, reasonable is a value close to one
	/// \note A small value leads to fragmentation, a value close to 1.0 leads to transactions slowing down
	bool filledWithRatio( float ratio) const
	{
		return size() >= (int)(ratio * Constants::maxPosInfoBlockSize());
	}

	const std::vector<DocIndexNode>& docIndexNodeArray() const	{return m_docIndexNodeArray;}
	const std::vector<PositionType>& posinfoArray() const		{return m_posinfoArray;}

	PosinfoBlock createBlock() const;
	void clear();

	int size() const
	{
		return (m_posinfoArray.size() * sizeof(PositionType)
				+ m_docIndexNodeArray.size() * sizeof(DocIndexNode));
	}

	static void merge( const PosinfoBlockBuilder& blk1, const PosinfoBlockBuilder& blk2, PosinfoBlockBuilder& newblk);

private:
	struct Cursor
	{
		std::size_t idx;
		const PosinfoBlockBuilder* blk;
		const DocIndexNode* nd;
		unsigned short subidx;
		strus::Index docno;

		explicit Cursor( const PosinfoBlockBuilder& blk_)
			:idx(0),blk(&blk_),nd(0),subidx(0),docno(0)
		{
			for (; !docno && idx < blk->docIndexNodes(); ++idx)
			{
				nd = blk->docIndexNodeRef( idx);
				docno = nd->firstDoc( subidx);
			}
		}

		void next()
		{
			docno = nd->nextDoc( subidx);
			while (!docno)
			{
				if (++idx < blk->docIndexNodes())
				{
					nd = blk->docIndexNodeRef( idx);
					docno = nd->firstDoc( subidx);
				}
				else
				{
					break;
				}
			}
		}

		const PositionType* posinfo() const
		{
			return blk->posinfo( nd->ref[ subidx]);
		}
	};


public:/*Cursor*/
	const DocIndexNode* docIndexNodeRef( std::size_t idx) const
	{
		return &m_docIndexNodeArray[ idx];
	}
	std::size_t docIndexNodes() const
	{
		return m_docIndexNodeArray.size();
	}
	const PositionType* posinfo( std::size_t ref) const
	{
		return m_posinfoArray.data() + ref;
	}

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<PositionType> m_posinfoArray;
	Index m_lastDoc;
	Index m_id;
};
}//namespace
#endif

