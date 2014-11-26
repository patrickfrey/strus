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
#ifndef _STRUS_LVDB_FORWARD_INDEX_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_FORWARD_INDEX_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "databaseKey.hpp"

namespace strus {

/// \class ForwardIndexBlock
/// \brief Block of term occurrence positions
class ForwardIndexBlock
	:public DataBlock
{
public:
	enum {
		DatabaseKeyPrefix=DatabaseKey::ForwardIndexPrefix,
		MaxBlockSize=1024
	};

public:
	explicit ForwardIndexBlock()
		:DataBlock( (char)DatabaseKeyPrefix){}
	ForwardIndexBlock( const ForwardIndexBlock& o)
		:DataBlock(o){}
	ForwardIndexBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)DatabaseKeyPrefix, id_, ptr_, size_){}

	ForwardIndexBlock& operator=( const ForwardIndexBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}

	void setId( const Index& id_);
	Index position_at( const char* ref) const;
	const char* value_at( const char* ref) const;

	Index relativeIndexFromPosition( const Index& pos_) const {return id()-pos_+1;}
	Index positionFromRelativeIndex( const Index& rel_) const {return id()-rel_+1;}

	const char* nextItem( const char* ref) const;
	const char* prevItem( const char* ref) const;

	const char* find( const Index& pos_, const char* lowerbound) const;
	const char* upper_bound( const Index& pos_, const char* lowerbound) const;

	bool full() const
	{
		return size() >= MaxBlockSize;
	}
	bool isThisBlockAddress( const Index& pos_) const
	{
		return (pos_ <= id() && pos_ > position_at( charptr()));
	}
	/// \brief Check if the address 'pos_', if it exists, probably is in the following block we can get with 'leveldb::Iterator::Next()' or not
	bool isFollowBlockAddress( const Index& pos_) const
	{
		Index diff = id() - position_at( charptr());
		return (pos_ > id()) && (pos_ < id() + diff - (diff >> 4));
	}

	void append( const Index& pos, const std::string& item);
};

}
#endif

