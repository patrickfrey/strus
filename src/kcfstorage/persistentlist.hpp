/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_KCF_PERSISTENT_LIST_HPP_INCLUDED
#define _STRUS_KCF_PERSISTENT_LIST_HPP_INCLUDED
#include "strus/position.hpp"
#include "blktable.hpp"
#include <cstdio>
#include <string>
#include <cstring>

namespace strus
{

///\class PersistentList
///\tparam Element
///\brief Implementation of a block map in a file
template <class Element>
class PersistentList
	:public BlockTable
{
public:
	enum {NofBlockElements=127};
	PersistentList( const std::string& type_, const std::string& name_, const std::string& path_, bool writemode_=false)
		:BlockTable( type_, sizeof(Block), name_, path_, writemode_)
		,m_lastidx(0)
	{
		std::memset( &m_cur, 0, sizeof( m_cur));
	}

	virtual ~PersistentList(){}

	static void create( const std::string& type_, const std::string& name_, const std::string& path_)
	{
		BlockTable::create( type_, sizeof(Block), name_, path_);
	}

	void open()
	{
		BlockTable::open();
		std::size_t nof_blocks = BlockTable::size();
		if (nof_blocks > 0)
		{
			m_lastidx = nof_blocks;
			readBlock( m_lastidx, &m_cur);
		}
		else
		{
			m_lastidx = 0;
			std::memset( &m_cur, 0, sizeof( m_cur));
		}
	}

	void push_back( const Element& element)
	{
		if (m_lastidx == 0 || m_cur.hdr.size == NofBlockElements)
		{
			Block newblk;
			std::memset( &newblk, 0, sizeof( newblk));
			newblk.hdr.size = 0;
			newblk.element[ newblk.hdr.size++] = element;
			m_lastidx = BlockTable::appendBlock( &newblk);
			std::memcpy( &m_cur, &newblk, sizeof( m_cur));
		}
		else
		{
			m_cur.element[ m_cur.hdr.size++] = element;
			try
			{
				writeBlock( m_lastidx, &m_cur);
			}
			catch (const std::runtime_error& e)
			{
				--m_cur.hdr.size;
				throw e;
			}
		}
	}

	void reset()
	{
		BlockTable::reset();
		std::memset( &m_cur, 0, sizeof( m_cur));
		m_lastidx = 0;
	}

private:
	struct Block
	{
		union
		{
			std::size_t size;
			Element _;
		} hdr;
		Element element[ NofBlockElements];
	};

public:
	class const_iterator
	{
	public:
		const_iterator( const PersistentList* ref_)
			:m_isopen(false),m_ref(ref_),m_curidx(0),m_curpos(0)
		{
			readNextBlock();
		}
		const_iterator()
			:m_isopen(false),m_ref(0),m_curidx(0),m_curpos(0)
		{}
		const_iterator( const const_iterator& o)
			:m_isopen(o.m_isopen),m_ref(o.m_ref),m_curidx(o.m_curidx),m_curpos(o.m_curpos)
		{
			std::memcpy( &m_cur, &o.m_cur, sizeof( m_cur));
		}

		const_iterator& operator++()
		{
			m_curpos++;
			if (m_curpos == m_cur.hdr.size)
			{
				if (!readNextBlock()) throw std::runtime_error( "illegal increment");
			}
		}

		const Element& operator*()
		{
			if (m_curpos == m_cur.hdr.size) throw std::runtime_error( "uninitialized memory read");
			return m_cur[ m_curpos];
		}

		bool operator==( const const_iterator& o) const	{return isequal(o);}
		bool operator!=( const const_iterator& o) const	{return !isequal(o);}

	private:
		bool isequal( const const_iterator& o) const
		{
			if (!o.m_ref)
			{
				if (!m_ref) return true;
				return (m_curidx == m_ref->m_lastidx && m_curpos == m_cur.hdr.size);
			}
			else if (!m_ref)
			{
				return o.isequal( *this);
			}
			return (m_curidx == o.m_curidx && m_curpos == o.m_curpos);
		}

		bool readNextBlock()
		{
			if (!m_isopen)
			{
				if (!m_ref->open()) throw std::runtime_error( m_ref->lastError());
			}
			if (m_curidx == m_ref->m_lastidx) return false;
			if (!m_ref->readBlock( m_curidx+1, &m_cur)) throw std::runtime_error( m_ref->lastError());
			m_curpos = 0;
			++m_curidx;
		}

	private:
		bool m_isopen;
		const PersistentList* m_ref;
		PersistentList::Block m_cur;
		Index m_curidx;
		std::size_t m_curpos;
	};

	const_iterator begin() const
	{
		return const_iterator(this);
	}
	const_iterator end() const
	{
		return const_iterator();
	}

private:
	friend class const_iterator;
	Block m_cur;
	Index m_lastidx;
};

} //namespace
#endif


