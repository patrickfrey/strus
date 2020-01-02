/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BOOLEAN_BLOCK_HPP_INCLUDED
#define _STRUS_STORAGE_BOOLEAN_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "strus/constants.hpp"
#include <vector>
#include <map>

namespace strus {

/// \class BooleanBlock
/// \brief Block for storing sets of integers
class BooleanBlock
	:public DataBlock
{
public:
	BooleanBlock()
		:m_first(0)
	{}

	BooleanBlock( const BooleanBlock& o)
		:DataBlock(o),m_first(o.m_first)
	{}

	BooleanBlock( strus::Index id_, const void* ptr_, std::size_t size_)
		:DataBlock( id_, ptr_, size_)
	{
		initFrame();
	}

	BooleanBlock& operator=( const BooleanBlock& o)
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

	struct NodeCursor
	{
		NodeCursor() :idx(0),elemno(0){}
		void reset() {idx=0;elemno=0;}

		std::size_t idx;
		Index elemno;
	};

	Index getFirstElem() const
	{
		return m_first;
	}
	Index getFirst( NodeCursor& cursor) const;
	Index getLast() const;
	Index getLast( NodeCursor& cursor) const;
	Index getNext( NodeCursor& cursor) const;
	Index skip( strus::Index elemno_, NodeCursor& cursor) const;

	bool getNextRange( NodeCursor& cursor, Index& from_, Index& to_) const;
	bool getFirstRange( NodeCursor& cursor, Index& from_, Index& to_) const;

	void defineElement( strus::Index elemno);
	void defineRange( strus::Index elemno, strus::Index rangesize);

	bool full() const
	{
		return (int)size() >= Constants::maxBooleanBlockSize();
	}
	/// \brief Eval if the block is filled with a given ratio
	/// \param[in] ratio value between 0.0 and 1.0, reasonable is a value close to one
	/// \note A small value leads to fragmentation, a value close to 1.0 leads to transactions slowing down
	bool filledWithRatio( float ratio) const
	{
		return (int)size() >= (int)(ratio * Constants::maxBooleanBlockSize());
	}

	/// \brief Check if the address 'elemno_', if it exists, is in this block.
	bool isThisBlockAddress( strus::Index elemno_) const
	{
		return (elemno_ <= id() && elemno_ >= getFirstElem());
	}

	/// \brief Check if the address 'elemno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( strus::Index elemno_) const
	{
		return (elemno_ > id() && elemno_ < id() + id() - getFirstElem());
	}

	struct MergeRange
	{
		strus::Index from;
		strus::Index to;
		bool isMember;

		MergeRange( strus::Index from_, strus::Index to_, bool isMember_)
			:from(from_),to(to_),isMember(isMember_){}
		MergeRange( const MergeRange& o)
			:from(o.from),to(o.to),isMember(o.isMember){}
	};

	static void merge( 
			std::vector<MergeRange>::const_iterator ei,
			const std::vector<MergeRange>::const_iterator& ee,
			const BooleanBlock& oldblk,
			BooleanBlock& newblk);
	static void merge_append(
			std::vector<MergeRange>::const_iterator ei,
			const std::vector<MergeRange>::const_iterator& ee,
			const BooleanBlock& oldblk,
			BooleanBlock& appendblk);

	static void merge( const BooleanBlock& blk1, const BooleanBlock& blk2, BooleanBlock& newblk);
	static void split( const BooleanBlock& blk, BooleanBlock& newblk1, BooleanBlock& newblk2);

	void check() const;

private:
	static bool joinRange( Index& from_, Index& to_, strus::Index addfrom_, strus::Index addto_);
	void initFrame();

private:
	struct Node
	{
		enum Type {DiffNode, PairNode};

		int type:2;
		int elemno:30;
		union {
			Index diff;
			Index elemno2;
		} alt;

		Index getFirstElem() const;
		Index getLastElem() const;
		Index getNextElem( strus::Index elemno_) const;
		Index getUpperBound( strus::Index elemno_) const;
		bool matches( strus::Index elemno_) const;

		void normalize();
		bool tryAddElem( strus::Index elemno_);
		bool tryExpandRange( strus::Index to_);
		void init( strus::Index from_, strus::Index to_);
		void getLastRange( Index& from_, Index& to_) const;
	};

private:
	strus::Index m_first;
};

}//namespace
#endif

