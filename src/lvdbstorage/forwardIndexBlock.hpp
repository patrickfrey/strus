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
		MaxBlockSize=1024
	};

public:
	explicit ForwardIndexBlock()
		:DataBlock( (char)DatabaseKey::ForwardIndexPrefix){}
	ForwardIndexBlock( const ForwardIndexBlock& o)
		:DataBlock(o){}
	ForwardIndexBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)DatabaseKey::ForwardIndexPrefix, id_, ptr_, size_){}

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
{
public:
	typedef ForwardIndexBlockElement mapped_type;

public:
	ForwardIndexBlockElementMap(){}
	ForwardIndexBlockElementMap( const ForwardIndexBlockElementMap& o)
		:m_map(o.m_map),m_strings(o.m_strings){}

	void define( const Index& idx, const ForwardIndexBlockElement& elem);

	Index lastInsertBlockId() const
	{
		return m_map.rbegin()->first;
	}

	void swap( ForwardIndexBlockElementMap& o)
	{
		m_map.swap( o.m_map);
		m_strings.swap( o.m_strings);
	}

	class IteratorElement
	{
	public:
		IteratorElement()
			:m_docno(0),m_ptr(0){}
		IteratorElement( const Index& docno_, const char* ptr_)
			:m_docno(docno_),m_ptr(ptr_){}
		IteratorElement( const IteratorElement& o)
			:m_docno(o.m_docno),m_ptr(o.m_ptr){}

		void init( const Index& docno_, const char* ptr_)
		{
			m_docno = docno_;
			m_ptr = ptr_;
		}

		const Index& key() const
		{
			return m_docno;
		}

		const char* value() const
		{
			return m_ptr;
		}

	private:
		Index m_docno;
		const char* m_ptr;
	};

	class const_iterator
	{
	public:
		const_iterator( const const_iterator& o)
			:m_elem(o.m_elem),m_map_itr(o.m_map_itr),m_strings_ref(o.m_strings_ref){}

		const_iterator( const ForwardIndexBlockElementMap* map_, bool start);

		bool operator==( const const_iterator& o) const	{return m_map_itr==o.m_map_itr;}
		bool operator!=( const const_iterator& o) const	{return m_map_itr!=o.m_map_itr;}

		const_iterator& operator++()			{++m_map_itr; m_elem.init( m_map_itr->first, m_strings_ref + m_map_itr->second); return *this;}
		const_iterator operator++(int)			{const_iterator rt(*this); ++m_map_itr; m_elem.init( m_map_itr->first, m_strings_ref + m_map_itr->second); return rt;}

		const IteratorElement& operator*() const	{return m_elem;}
		const IteratorElement* operator->() const	{return &m_elem;}

	private:
		IteratorElement m_elem;
		std::map<Index,std::size_t>::const_iterator m_map_itr;
		const char* m_strings_ref;
	};

	const_iterator begin() const
	{
		return const_iterator( this, true);
	}
	const_iterator end() const
	{
		return const_iterator( this, false);
	}

	std::map<Index,std::size_t>::const_iterator map_begin() const
	{
		return m_map.begin();
	}
	std::map<Index,std::size_t>::const_iterator map_end() const
	{
		return m_map.end();
	}
	const char* strings_ptr() const
	{
		return m_strings.c_str();
	}

private:
	std::map<Index,std::size_t> m_map;
	std::string m_strings;
};

}
#endif

