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
#include "strus/index.hpp"
#include <leveldb/db.h>
#include <stdint.h>

namespace strus {

/// \class PosinfoBlock
/// \brief Block of term occurrence positions of some documents
/// \note A posinfo block is a sequence of variable length elements 
///	starting with the relative document number (offset to subract
///	from the upper_bound document number of the block) followed by
///	encoded position numbers and ending with the byte 0xFF
class PosinfoBlock
{
public:
	class Element
	{
	public:
		Element() :m_ptr(0),m_itr(0),m_end(0),m_docno(0),m_posno(0){}
		Element( const Index& docno_, const char* ptr_, std::size_t size_)
			:m_ptr(ptr_),m_itr(ptr_),m_end(ptr_+size_),m_docno((uint32_t)docno_),m_posno(0){}
		Element( const Element& o)
			:m_ptr(o.m_ptr),m_itr(o.m_itr),m_end(o.m_end),m_docno(o.m_docno),m_posno(o.m_posno){}

		bool empty() const
		{
			return !m_ptr;
		}

		Index skipPos( const Index& pos_);

		Index docno() const
		{
			return m_docno;
		}

		const char* end() const
		{
			return m_end;
		}

		void init( const Index& docno_, const char* ptr_, std::size_t size_)
		{
			m_ptr = ptr_;
			m_itr = ptr_;
			m_end = ptr_+size_;
			m_docno = (uint32_t)docno_;
			m_posno = 0;
		}

	private:
		const char* m_ptr;
		const char* m_itr;
		const char* m_end;
		uint32_t m_docno;
		uint32_t m_posno;
	};

public:
	PosinfoBlock()
		:m_docno(0),m_blkptr(0),m_blkend(0),m_blkitr(0){}
	PosinfoBlock( const PosinfoBlock& o)
		:m_docno(o.m_docno),m_blkptr(o.m_blkptr),m_blkend(o.m_blkend),m_blkitr(o.m_blkitr),m_elem(o.m_elem){}

	PosinfoBlock( const Index& docno_, const char* ptr_, std::size_t size_)
		:m_docno(docno_),m_blkptr(ptr_),m_blkend(ptr_+size_),m_blkitr(ptr_){}

	const Element* find( const Index& docno_) const;
	const Element* upper_bound( const Index& docno_) const;

	void init( const Index& docno_, const char* ptr_, std::size_t size_)
	{
		m_docno = docno_;
		m_blkptr = ptr_;
		m_blkend = ptr_+size_;
		m_blkitr = ptr_;
	}

private:
	void loadElement();

private:
	Index m_docno;
	const char* m_blkptr;
	const char* m_blkend;
	char const* m_blkitr;
	Element m_elem;
};

}
#endif

