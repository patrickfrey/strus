/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_LVDB_BOOLEAN_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_BOOLEAN_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include <vector>
#include <map>

namespace strus {

/// \class BooleanBlock
/// \brief Block for storing sets of integers
class BooleanBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockSize=1024
	};

public:
	BooleanBlock()
		:m_first(0)
	{}

	BooleanBlock( const BooleanBlock& o)
		:DataBlock(o),m_first(o.m_first)
	{}

	BooleanBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( id_, ptr_, size_)
	{
		initFrameData();
	}

	BooleanBlock& operator=( const BooleanBlock& o)
	{
		DataBlock::operator =(o);
		initFrameData();
		return *this;
	}

	void swap( DataBlock& o)
	{
		DataBlock::swap( o);
		initFrameData();
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
	Index skip( const Index& elemno_, NodeCursor& cursor) const;

	bool getNextRange( NodeCursor& cursor, Index& from_, Index& to_) const;
	bool getFirstRange( NodeCursor& cursor, Index& from_, Index& to_) const;

	void defineElement( const Index& elemno);
	void defineRange( const Index& elemno, const Index& rangesize);

	bool full() const
	{
		return size() >= MaxBlockSize;
	}

	/// \brief Check if the address 'elemno_', if it exists, is in this block.
	bool isThisBlockAddress( const Index& elemno_) const
	{
		return (elemno_ <= id() && elemno_ >= getFirstElem());
	}

	/// \brief Check if the address 'elemno_', if it exists, is most likely located in the following block (cheaper to fetch) or not
	bool isFollowBlockAddress( const Index& elemno_) const
	{
		return (elemno_ > id() && elemno_ < id() + id() - getFirstElem());
	}

	struct MergeRange
	{
		Index from;
		Index to;
		bool isMember;

		MergeRange( const Index& from_, const Index to_, bool isMember_)
			:from(from_),to(to_),isMember(isMember_){}
		MergeRange( const MergeRange& o)
			:from(o.from),to(o.to),isMember(o.isMember){}
	};

	static void merge( 
			std::vector<MergeRange>::const_iterator ei,
			const std::vector<MergeRange>::const_iterator& ee,
			const BooleanBlock& oldblk,
			BooleanBlock& newblk);

	void check() const;

private:
	static bool joinRange( Index& from_, Index& to_, const Index& addfrom_, const Index& addto_);
	void initFrameData();

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
		Index getNextElem( const Index& elemno_) const;
		Index getUpperBound( const Index& elemno_) const;
		bool matches( const Index& elemno_) const;

		void normalize();
		bool tryAddElem( const Index& elemno_);
		void init( const Index& from_, const Index& to_);
		void getLastRange( Index& from_, Index& to_) const;
	};

private:
	Index m_first;
};

}//namespace
#endif

