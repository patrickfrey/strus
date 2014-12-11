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
	std::string value_at( const char* ref) const;

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


typedef std::string ForwardIndexBlockElement;

class ForwardIndexBlockElementMap
	:public std::map<Index,ForwardIndexBlockElement>
{
public:
	ForwardIndexBlockElementMap(){}
	ForwardIndexBlockElementMap( const ForwardIndexBlockElementMap& o)
		:std::map<Index,ForwardIndexBlockElement>(o){}

	void define( const Index& idx, const ForwardIndexBlockElement& elem)
	{
		std::map<Index,ForwardIndexBlockElement>::operator[]( idx) = elem;
	}

	Index lastInsertBlockId() const
	{
		return rbegin()->first;
	}

	class IteratorElement
	{
	public:
		explicit IteratorElement( const std::map<Index,ForwardIndexBlockElement>::const_iterator& itr_)
			:m_itr(itr_){}
		IteratorElement& operator=( const std::map<Index,ForwardIndexBlockElement>::const_iterator& itr_)
		{
			m_itr = itr_;
			return *this;
		}

		const Index& key() const			{return m_itr->first;}
		const ForwardIndexBlockElement& value() const	{return m_itr->second;}

	private:
		std::map<Index,ForwardIndexBlockElement>::const_iterator m_itr;
	};

	class const_iterator
	{
	public:
		const_iterator( const const_iterator& o)
			:m_itr(o.m_itr),m_elem(o.m_itr){}

		explicit const_iterator( const std::map<Index,ForwardIndexBlockElement>::const_iterator& itr_)
			:m_itr(itr_),m_elem(itr_){}

		bool operator==( const const_iterator& o) const	{return m_itr==o.m_itr;}
		bool operator!=( const const_iterator& o) const	{return m_itr!=o.m_itr;}

		const_iterator& operator++()			{++m_itr; m_elem = m_itr; return *this;}
		const_iterator operator++(int)			{const_iterator rt(*this); ++m_itr; m_elem = m_itr; return rt;}

		const IteratorElement& operator*() const	{return m_elem;}
		const IteratorElement* operator->() const	{return &m_elem;}

	private:
		std::map<Index,ForwardIndexBlockElement>::const_iterator m_itr;
		IteratorElement m_elem;
	};

	const_iterator begin() const
	{
		return const_iterator( std::map<Index,ForwardIndexBlockElement>::begin());
	}
	const_iterator end() const
	{
		return const_iterator( std::map<Index,ForwardIndexBlockElement>::end());
	}
};

}
#endif

