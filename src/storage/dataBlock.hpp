/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_DATA_BLOCK_HPP_INCLUDED
#define _STRUS_STORAGE_DATA_BLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstdlib>
#include <stdexcept>

namespace strus {

/// \class DataBlock
class DataBlock
{
public:
	DataBlock()
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
	const void* data_at( int idx)	{return (const void*)((const char*)m_ptr + idx);}

	void init( const Index& id_, const void* ptr_, std::size_t size_, std::size_t allocsize_=0);
	void init( const Index& id_, std::size_t allocsize_);

	void append( const void* data, std::size_t datasize);
	void fill( char ch, std::size_t datasize);

	void setByte( std::size_t idx, unsigned char elem)
	{
		if (idx >= m_size) throw strus::runtime_error( _TXT( "array bound write (%s)"), __FUNCTION__);
		m_ptr[ idx] = elem;
	}
	void unionByte( std::size_t idx, unsigned char elem)
	{
		if (idx >= m_size) throw strus::runtime_error( _TXT( "array bound write (%s)"), __FUNCTION__);
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

