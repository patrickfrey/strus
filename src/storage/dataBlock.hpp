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
#ifndef _STRUS_LVDB_DATA_BLOCK_HPP_INCLUDED
#define _STRUS_LVDB_DATA_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include <cstdlib>
#include <stdexcept>

namespace strus {

/// \class DataBlock
class DataBlock
{
public:
	explicit DataBlock()
		:m_id(0),m_ptr(0),m_size(0),m_allocsize(0)
	{}

	DataBlock( const Index& id_, const void* ptr_, std::size_t size_, bool allocated_=false)
		:m_id(0),m_ptr(0),m_size(0),m_allocsize(0)
	{
		init( id_, ptr_, size_, allocated_?size_:0);
	}

	DataBlock( const Index& id_, std::size_t size_)
		:m_id(0),m_ptr(0),m_size(0),m_allocsize(0)
	{
		init( id_, size_);
	}

	DataBlock( const DataBlock& o)
		:m_id(0),m_ptr(0),m_size(0),m_allocsize(0)
	{
		init( o.m_id, o.m_ptr, o.m_size, o.m_allocsize);
	}

	virtual ~DataBlock();

	DataBlock& operator=( const DataBlock& o)
	{
		init( o.m_id, o.m_ptr, o.m_size, o.m_allocsize);
		return *this;
	}
	void swap( DataBlock& o);

	void clear()			{m_size=0;}
	bool empty() const		{return !m_size;}
	Index id() const		{return m_id;}
	void setId( const Index& id_)	{m_id = id_;}
	std::size_t size() const	{return m_size;}
	const void* end() const		{return m_ptr + m_size;}
	const void* ptr() const		{return m_ptr;}
	const char* charptr() const	{return (const char*)m_ptr;}
	const char* charend() const	{return (const char*)m_ptr + m_size;}

	void init( const Index& id_, const void* ptr_, std::size_t size_, std::size_t allocsize_=0);
	void init( const Index& id_, std::size_t allocsize_);

	void append( const void* data, std::size_t datasize);
	void fill( char ch, std::size_t datasize);

	void setByte( std::size_t idx, unsigned char elem)
	{
		if (idx >= m_size) throw std::logic_error("array bound write (DataBlock::setByte)");
		m_ptr[ idx] = elem;
	}
	void unionByte( std::size_t idx, unsigned char elem)
	{
		if (idx >= m_size) throw std::logic_error("array bound write (DataBlock::setByte)");
		m_ptr[ idx] |= elem;
	}
	void resize( std::size_t newsize_)
	{
		if (newsize_ > m_size)
		{
			fill( '\0', newsize_ - m_size);
		}
		else
		{
			m_size = newsize_;
		}
	}

private:
	void expand( std::size_t datasize);

private:
	Index m_id;
	char* m_ptr;
	std::size_t m_size;
	std::size_t m_allocsize;
};

} //namespace
#endif

