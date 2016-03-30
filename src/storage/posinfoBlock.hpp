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
#include <vector>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};
	typedef unsigned short PositionType;

public:
	explicit PosinfoBlock()
		:DataBlock(),m_docindexptr(0),m_posinfoptr(0)
	{}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o)
		{initDocIndexNodeFrame();}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
		{initDocIndexNodeFrame();}

	PosinfoBlock& operator=( const PosinfoBlock& o)
	{
		DataBlock::operator =(o);
		initDocIndexNodeFrame();
		return *this;
	}
	void swap( DataBlock& o)
	{
		DataBlock::swap( o);
		initDocIndexNodeFrame();
	}

	struct Cursor
	{
		unsigned short nodeidx;
		unsigned short docidx;

		Cursor()
			:nodeidx(0),docidx(0){}
		Cursor( const Cursor& o)
			:nodeidx(o.nodeidx),docidx(o.docidx){}
		void reset()
		{
			nodeidx = 0;
			docidx = 0;
		}
	};

	/// \brief Get the document number of the current PosinfoBlock::Cursor
	Index docno_at( const Cursor& cursor) const;
	/// \brief Get the internal representation of the postions of the current PosinfoBlock::Cursor
	const PositionType* posinfo_at( const Cursor& cursor) const;
	/// \brief Get the postions of the current Cursor
	std::vector<Index> positions_at( const Cursor& cursor) const;
	/// \brief Get the feature frequency of the current PosinfoBlock::Cursor
	unsigned int frequency_at( const Cursor& cursor) const;

	/// \brief Get the next document with the current cursor
	Index nextDoc( Cursor& cursor) const;
	/// \brief Get the first document with the current cursor
	Index firstDoc( Cursor& cursor) const;
	/// \brief Upper bound search for a docnument number in the block
	Index skipDoc( const Index& docno_, Cursor& cursor) const;

	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && m_nofDocIndexNodes && docno_ > m_docindexptr[ 0].base);
	}
	/// \brief Check if the address 'docno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - (m_nofDocIndexNodes?m_docindexptr[ 0].base:1);
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

	PositionScanner positionScanner_at( const Cursor& cursor) const
	{
		return PositionScanner( posinfo_at( cursor));
	}

public:/*PosinfoBlockBuilder*/
	struct DocIndexNode
	{
		enum {Size=7};
		DocIndexNode();
		DocIndexNode( const DocIndexNode& o);

		bool addDocument( const Index& docno_, unsigned short posrefIdx_);
		Index skipDoc( const Index& docno_, unsigned short& cursor_docidx) const;
		std::size_t nofElements() const;

		Index base;
		unsigned short ofs[ Size-1];
		unsigned short posrefIdx[ Size];
	};

private:
	void initDocIndexNodeFrame();

private:
	std::size_t m_nofDocIndexNodes;
	const DocIndexNode* m_docindexptr;
	const PositionType* m_posinfoptr;
};

class PosinfoBlockBuilder
{
public:
	typedef PosinfoBlock::PositionType PositionType;
	typedef PosinfoBlock::DocIndexNode DocIndexNode;

public:
	PosinfoBlockBuilder( const PosinfoBlock& o);
	PosinfoBlockBuilder()
		:m_lastDoc(0),m_id(0){}
	PosinfoBlockBuilder( const PosinfoBlockBuilder& o)
		:m_docIndexNodeArray(o.m_docIndexNodeArray)
		,m_posinfoArray(o.m_posinfoArray)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}

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
		return (m_posinfoArray.size() * sizeof(PositionType)
				+ m_docIndexNodeArray.size() * sizeof(DocIndexNode))
			>= PosinfoBlock::MaxBlockSize;
	}

	const std::vector<DocIndexNode>& docIndexNodeArray() const	{return m_docIndexNodeArray;}
	const std::vector<PositionType>& posinfoArray() const		{return m_posinfoArray;}

	PosinfoBlock createBlock() const;
	void clear();

private:
	std::vector<DocIndexNode> m_docIndexNodeArray;
	std::vector<PositionType> m_posinfoArray;
	Index m_lastDoc;
	Index m_id;
};
}//namespace
#endif

