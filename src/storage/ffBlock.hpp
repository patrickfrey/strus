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

namespace strus {

/// \class FfBlock
/// \brief Block for fast access of ff for initial query evaluation
class FfBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};

public:
	explicit FfBlock()
		:DataBlock(),m_ffIndexNodeArray()
	{}
	FfBlock( const FfBlock& o)
		:DataBlock(o)
		{initFrame();}
	FfBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
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

	/// \brief Get the document number of the current FfIndexNodeCursor
	Index docno_at( const FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.docno_at( cursor);
	}

	/// \brief Get the feature frequency of the current DocIndexNodeCursor
	int frequency_at( const FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.ff_at( cursor);
	}

	/// \brief Get the next document with the current cursor
	Index nextDoc( FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.nextDoc( cursor);
	}
	/// \brief Get the first document with the current cursor
	Index firstDoc( FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.firstDoc( cursor);
	}
	/// \brief Upper bound search for a docnument number in the block
	Index skipDoc( const Index& docno_, FfIndexNodeCursor& cursor) const
	{
		return m_ffIndexNodeArray.skipDoc( docno_, cursor);
	}

	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && m_ffIndexNodeArray.size && docno_ > m_ffIndexNodeArray.ar[ 0].base);
	}
	/// \brief Check if the address 'docno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - (m_ffIndexNodeArray.size?m_ffIndexNodeArray.ar[ 0].base:1);
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
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

	Index id() const						{return m_id;}
	void setId( const Index& id_);

	bool empty() const						{return m_ffIndexNodeArray.empty();}

	/// \brief Append document with ff
	/// \param[in] docno document number
	/// \param[in] ff term feature frequency (ff, number of occurrencies) in the document
	void append( const Index& docno, int ff);

	bool full() const
	{
		return (m_ffIndexNodeArray.size() * sizeof(FfIndexNode)) >= Constants::maxFfBlockSize();
	}

	const std::vector<FfIndexNode>& ffIndexNodeArray() const	{return m_ffIndexNodeArray;}

	FfBlock createBlock() const;

	void clear()
	{
		m_ffIndexNodeArray.clear();
		m_lastDoc = 0;
		m_id = 0;
	}

private:
	std::vector<FfIndexNode> m_ffIndexNodeArray;
	Index m_lastDoc;
	Index m_id;
};

}//namespace
#endif

