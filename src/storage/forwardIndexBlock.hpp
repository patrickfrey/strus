/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
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
#include "private/localStructAllocator.hpp"
#include <string>
#include <map>

namespace strus {

/// \class ForwardIndexBlock
/// \brief Block of term occurrence positions
class ForwardIndexBlock
	:public DataBlock
{
public:
	enum {
		MaxBlockTokens=128
	};

public:
	ForwardIndexBlock(){}
	ForwardIndexBlock( const ForwardIndexBlock& o)
		:DataBlock(o){}
	ForwardIndexBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( id_, ptr_, size_){}

	ForwardIndexBlock& operator=( const ForwardIndexBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}

	void setId( const Index& id_);
	Index position_at( const char* ref) const;
	std::string value_at( const char* ref) const;

	Index relativeIndexFromPosition( const Index& pos_) const {return id()-pos_+1;}
	Index positionFromRelativeIndex( const Index& rel_) const {return id()-rel_+1;}

	const char* nextItem( const char* ref) const;
	const char* prevItem( const char* ref) const;

	const char* find( const Index& pos_, const char* lowerbound) const;
	const char* upper_bound( const Index& pos_, const char* lowerbound) const;

	void append( const Index& pos, const std::string& item);

	class const_iterator
	{
	public:
		const_iterator( const ForwardIndexBlock* blk_, const char* ref_)
			:m_blk(blk_),m_ref(ref_){}
		const_iterator( const const_iterator& o)
			:m_blk(o.m_blk),m_ref(o.m_ref){}

		std::string operator*() const			{return m_blk->value_at( m_ref);}

		const_iterator& operator++()			{m_ref = m_blk->nextItem( m_ref); return *this;}
		const_iterator operator++(int)			{const_iterator rt(m_blk,m_ref); m_ref = m_blk->nextItem( m_ref); return rt;}

		bool operator==( const const_iterator& o) const	{return m_ref == o.m_ref;}
		bool operator!=( const const_iterator& o) const	{return m_ref != o.m_ref;}

	private:
		const ForwardIndexBlock* m_blk;
		char const* m_ref;
	};

	const_iterator begin() const
	{
		return const_iterator( this, charptr());
	}
	const_iterator end() const
	{
		return const_iterator( this, charend());
	}
};

}
#endif

