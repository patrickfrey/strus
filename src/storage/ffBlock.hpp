/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_FF_BLOCK_HPP_INCLUDED
#define _STRUS_FF_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "ffIndexNode.hpp"
#include "strus/constants.hpp"
#include <vector>
#include <algorithm>

namespace strus {

/// \class FfBlock
/// \brief Block for fast access of ff for initial query evaluation
class FfBlock
	:public DataBlock
{
public:
	explicit FfBlock()
		:DataBlock(),m_ffIndexNodeArray()
	{}
	FfBlock( const FfBlock& o)
		:DataBlock(o)
		{initFrame();}
	FfBlock( const strus::Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:DataBlock( id_, ptr_, size_, allocated_)
		{initFrame();}

	FfBlock& operator=( const FfBlock& o)
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

	bool full() const
	{
		return (int)size() >= Constants::maxFfBlockSize();
	}

	/// \brief Get the document number of the current FfIndexNodeCursor
	strus::Index docno_at( const FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.docno_at( cursor);
	}

	/// \brief Get the feature frequency of the current DocIndexNodeCursor
	int frequency_at( const FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.ff_at( cursor);
	}

	/// \brief Get the next document with the current cursor
	strus::Index nextDoc( FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.nextDoc( cursor);
	}
	/// \brief Get the first document and initialize the current cursor
	strus::Index firstDoc( FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.firstDoc( cursor);
	}

	/// \brief Get the last document
	strus::Index lastDoc() const
	{
		return m_ffIndexNodeArray.lastDoc();
	}

	/// \brief Upper bound search for a docnument number in the block
	strus::Index skipDoc( const strus::Index& docno, FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.skipDoc( docno, cursor);
	}

	bool isThisBlockAddress( const strus::Index& docno) const
	{
		return (docno <= id() && m_ffIndexNodeArray.size && docno > m_ffIndexNodeArray.ar[ 0].firstDoc());
	}
	/// \brief Check if the address 'docno', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const strus::Index& docno) const
	{
		Index diff = id() - (m_ffIndexNodeArray.size?m_ffIndexNodeArray.ar[ 0].firstDoc():1);
		return (docno > id()) && (docno < id() + diff - (diff>>4));
	}

	const FfIndexNodeArray& nodes() const
	{
		return m_ffIndexNodeArray;
	}

private:
	void initFrame();

private:
	FfIndexNodeArray m_ffIndexNodeArray;
};


class FfBlockBuilder
{
public:
	FfBlockBuilder( const FfBlock& o)
		:m_ffIndexNodeArray(o.nodes().tovector())
		,m_lastDoc(o.nodes().lastDoc()),m_id(o.nodes().lastDoc()){}
	FfBlockBuilder()
		:m_ffIndexNodeArray()
		,m_lastDoc(0),m_id(0){}
	FfBlockBuilder( const FfBlockBuilder& o)
		:m_ffIndexNodeArray(o.m_ffIndexNodeArray)
		,m_lastDoc(o.m_lastDoc)
		,m_id(o.m_id){}
	FfBlockBuilder& operator=( const FfBlockBuilder& o)
	{
		m_ffIndexNodeArray = o.m_ffIndexNodeArray;
		m_lastDoc = o.m_lastDoc;
		m_id = o.m_id;
		return *this;
	}

	strus::Index id() const						{return m_id;}
	void setId( const strus::Index& id_);

	bool empty() const						{return m_ffIndexNodeArray.empty();}

	/// \brief Append document with ff
	/// \param[in] docno document number
	/// \param[in] ff term feature frequency (ff, number of occurrencies) in the document
	void append( const strus::Index& docno, int ff);

	bool full() const
	{
		return size() >= Constants::maxFfBlockSize();
	}
	/// \brief Eval if the block is filled with a given ratio
	/// \param[in] ratio value between 0.0 and 1.0, reasonable is a value close to one
	/// \note A small value leads to fragmentation, a value close to 1.0 leads to transactions slowing down
	bool filledWithRatio( float ratio) const
	{
		return size() >= (int)(ratio * Constants::maxFfBlockSize());
	}

	const std::vector<FfIndexNode>& ffIndexNodeArray() const	{return m_ffIndexNodeArray;}

	FfBlock createBlock() const;

	void clear()
	{
		m_ffIndexNodeArray.clear();
		m_lastDoc = 0;
		m_id = 0;
	}

	int size() const
	{
		return sizeof(int) + (m_ffIndexNodeArray.size() * sizeof(FfIndexNode));
	}
	strus::Index lastDoc() const
	{
		return m_ffIndexNodeArray.empty() ? 0 : m_ffIndexNodeArray.back().lastDoc();
	}

	struct FfDeclaration
	{
		strus::Index docno;
		int ff;

		FfDeclaration( strus::Index docno_, int ff_)
			:docno(docno_),ff(ff_){}
		FfDeclaration( const FfDeclaration& o)
			:docno(o.docno),ff(o.ff){}
	};

	static void merge(
			std::vector<FfDeclaration>::const_iterator ei,
			const std::vector<FfDeclaration>::const_iterator& ee,
			const FfBlock& oldblk,
			FfBlockBuilder& newblk);
	static void merge_append(
			std::vector<FfDeclaration>::const_iterator ei,
			const std::vector<FfDeclaration>::const_iterator& ee,
			const FfBlock& oldblk,
			FfBlockBuilder& appendblk);
	static void merge( const FfBlockBuilder& blk1, const FfBlockBuilder& blk2, FfBlockBuilder& newblk);
	static void split( const FfBlockBuilder& blk, FfBlockBuilder& newblk1, FfBlockBuilder& newblk2);

	void swap( FfBlockBuilder& o)
	{
		m_ffIndexNodeArray.swap( o.m_ffIndexNodeArray);
		std::swap( m_lastDoc, o.m_lastDoc);
		std::swap( m_id, o.m_id);
	}

private:
	std::vector<FfIndexNode> m_ffIndexNodeArray;
	strus::Index m_lastDoc;
	strus::Index m_id;
};

}//namespace
#endif

