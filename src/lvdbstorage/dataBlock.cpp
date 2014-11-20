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
#include "dataBlock.hpp"
#include <cstring>

using namespace strus;

void DataBlock::init( const Index& id_, const void* ptr_, std::size_t size_, std::size_t allocsize_)
{
	if (m_allocsize)
	{
		std::free( m_ptr);
	}
	if (allocsize_)
	{
		std::size_t mm = ((allocsize_ >> 10)+1) << 10;
		m_ptr = (char*)std::malloc( mm);
		if (!m_ptr) throw std::bad_alloc();
		m_allocsize = mm;
		std::memcpy( m_ptr, ptr_, size_);
	}
	else
	{
		m_ptr = static_cast<char*>(const_cast<void*>( ptr_));
		m_allocsize = 0;
	}
	m_size = size_;
	m_id = id_;
}

void DataBlock::initcopy( const DataBlock& o)
{
	if (m_type != o.m_type) throw std::logic_error( "block type mismatch in initcopy");
	std::size_t mm = ((o.m_size >> 10)+1) << 10;
	void* newptr = std::malloc( mm);
	if (!newptr) throw std::bad_alloc();
	std::memcpy( newptr, m_ptr, m_size);
	if (m_allocsize)
	{
		std::free( m_ptr);
	}
	m_ptr = (char*)newptr;
	m_allocsize = mm;
	m_size = o.m_size;
	m_id = o.m_id;
}

void DataBlock::append( const void* data, std::size_t datasize)
{
	if (datasize + m_size > m_allocsize)
	{
		std::size_t mm = (((datasize +m_size) >> 10)+1) << 10;
		char* mp;
		if (m_allocsize)
		{
			mp = (char*)std::realloc( m_ptr, mm);
			if (!mp) throw std::bad_alloc();
		}
		else
		{
			mp = (char*)std::malloc( mm);
			if (!mp) throw std::bad_alloc();
			std::memcpy( mp, m_ptr, m_size);
		}
		m_allocsize = mm;
		m_ptr = mp;
	}
	std::memcpy( m_ptr+m_size, data, datasize);
	m_size += datasize;
}


