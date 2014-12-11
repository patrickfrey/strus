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
#ifndef _STRUS_LVDB_POSINFO_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_POSINFO_BLOCK_HPP_INCLUDED
#include "dataBlock.hpp"
#include "databaseKey.hpp"
#include <vector>
#include <map>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions
class PosinfoBlock
	:public DataBlock
{
public:
	enum {
		DatabaseKeyPrefix=DatabaseKey::PosinfoBlockPrefix,
		MaxBlockSize=1024
	};

public:
	explicit PosinfoBlock()
		:DataBlock( (char)DatabaseKeyPrefix){}
	PosinfoBlock( const PosinfoBlock& o)
		:DataBlock(o){}
	PosinfoBlock( const Index& id_, const void* ptr_, std::size_t size_)
		:DataBlock( (char)DatabaseKeyPrefix, id_, ptr_, size_){}

	PosinfoBlock& operator=( const PosinfoBlock& o)
	{
		DataBlock::operator =(o);
		return *this;
	}

	void setId( const Index& id_);
	Index docno_at( const char* ref) const;
	std::vector<Index> positions_at( const char* itr) const;
	unsigned int frequency_at( const char* itr) const;
	bool empty_at( const char* itr) const;
	const char* end_at( const char* itr) const;

	const char* begin() const
	{
		return charptr();
	}
	const char* end() const
	{
		return charend();
	}

	const char* endOfDoc( const char* ref) const;
	const char* nextDoc( const char* ref) const;
	const char* prevDoc( const char* ref) const;

	Index relativeIndexFromDocno( const Index& docno_) const {return id()-docno_+1;}
	Index docnoFromRelativeIndex( const Index& dcidx_) const {return id()-dcidx_+1;}

	const char* find( const Index& docno_, const char* lowerbound) const;
	const char* upper_bound( const Index& docno_, const char* lowerbound) const;

	bool full() const
	{
		return size() >= MaxBlockSize;
	}
	bool isThisBlockAddress( const Index& docno_) const
	{
		return (docno_ <= id() && docno_ > docno_at( begin()));
	}
	/// \brief Check if the address 'docno_', if it exists, probably is in the following block we can get with 'leveldb::Iterator::Next()' or not
	bool isFollowBlockAddress( const Index& docno_) const
	{
		Index diff = id() - docno_at( begin());
		return (docno_ > id()) && (docno_ < id() + diff - (diff>>4));
	}

	void append( const Index& docno, const std::vector<Index>& pos);
	void append( const Index& docno, const char* posinfo);
	void appendPositionsBlock( const char* start, const char* end);

	static PosinfoBlock merge( const PosinfoBlock& newblk, const PosinfoBlock& oldblk);

	class PositionScanner
	{
	public:
		PositionScanner()
			:m_start(0),m_end(0),m_itr(0),m_curpos(0),m_lastpos(0){}
		PositionScanner( const char* start_, const char* end_)
			:m_start(start_),m_end(end_),m_itr(start_),m_curpos(0),m_lastpos(0){}
		PositionScanner( const PositionScanner& o)
			:m_start(o.m_start),m_end(o.m_end),m_itr(o.m_itr),m_curpos(o.m_curpos),m_lastpos(o.m_lastpos){}

		bool initialized() const				{return !!m_itr;}

		void init( const char* start_, const char* end_)
		{
			m_itr = start_;
			m_start = start_;
			m_end = end_;
			m_curpos = 0;
			m_lastpos = 0;
		}

		void clear()						{init(0,0);}

		Index curpos() const					{return m_curpos;}
		Index skip( const Index& pos);

	private:
		const char* m_start;
		const char* m_end;
		char const* m_itr;
		Index m_curpos;
		Index m_lastpos;
	};

	PositionScanner positionScanner_at( const char* itr) const;
};


typedef std::vector<Index> PosinfoBlockElement;

std::ostream& operator<< (std::ostream& out, const PosinfoBlockElement& e);


class PosinfoBlockElementMap
{
public:
	typedef PosinfoBlockElement mapped_type;

public:
	PosinfoBlockElementMap(){}
	PosinfoBlockElementMap( const PosinfoBlockElementMap& o)
		:m_map(o.m_map){}

	void define( const Index& idx, const PosinfoBlockElement& elem);

	Index lastInsertBlockId() const
	{
		return m_map.rbegin()->first;
	}

	void swap( PosinfoBlockElementMap& o)
	{
		m_map.swap( o.m_map);
		m_strings.swap( o.m_strings);
	}

public:
	class IteratorElement
	{
	public:
		IteratorElement()
			:m_docno(0),m_ptr(0){}
		IteratorElement( const Index& docno_, const char* ptr_, std::size_t size_)
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

public:
	class const_iterator
	{
	public:
		const_iterator( const const_iterator& o)
			:m_elem(o.m_elem),m_map_itr(o.m_map_itr),m_strings_ref(o.m_strings_ref){}

		const_iterator( const PosinfoBlockElementMap* map_, bool start);

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

