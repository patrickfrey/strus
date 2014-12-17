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
#include "databaseKey.hpp"
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
	BooleanBlock( char dbkeyprefix)
		:DataBlock( dbkeyprefix)
	{}

	BooleanBlock( const BooleanBlock& o)
		:DataBlock(o)
	{}

	BooleanBlock( char dbkeyprefix, const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( dbkeyprefix, id_, ptr_, size_)
	{}

	BooleanBlock& operator=( const BooleanBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}
	const char* find( const Index& docno_, const char* lowerbound) const;
	const char* upper_bound( const Index& docno_, const char* lowerbound) const;

	bool full() const
	{
		return size() >= MaxBlockSize;
	}

	void defineElement( const Index& elemno);
	void defineRange( const Index& elemno, const Index& rangesize);

	bool getNextRange( char const*& itr, Index& from_, Index& to_) const;
	bool getLastRange( std::size_t& at_, Index& from_, Index& to_) const;
	static bool joinRange( Index& from_, Index& to_, const Index& addfrom_, const Index& addto_);

private:
	Index relativeIndexFromElemno( const Index& elemno_) const	{return id()-elemno_+1;}
	Index elemnoFromRelativeIndex( const Index& eidx_) const	{return id()-eidx_+1;}
};


class BooleanBlockElementMap
	:public std::map<Index,bool>
{
public:
	BooleanBlockElementMap(){}
	BooleanBlockElementMap( const BooleanBlockElementMap& o)
		:std::map<Index,bool>(o){}

	static BooleanBlock merge( const_iterator ei, const const_iterator& ee, const BooleanBlock& oldblk);
};

}//namespace
#endif

