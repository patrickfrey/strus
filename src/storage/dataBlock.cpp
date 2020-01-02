/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "dataBlock.hpp"
#include <cstring>
#include <stdexcept>
#include <map>
#include <iostream>
#include <algorithm>

using namespace strus;

#define BLOCK_ALLOC_SIZE(minNofBytes)  (((((minNofBytes) + 1024-1) / 1024)) * 1024)

DataBlock::~DataBlock()
{
	if (m_allocsize) std::free( m_ptr);
}

void DataBlock::init( strus::Index id_, const void* ptr_, std::size_t size_, std::size_t allocsize_)
{
	if (allocsize_)
	{
		std::size_t mm = BLOCK_ALLOC_SIZE( size_);
		void* newptr = std::malloc( mm);
		if (!newptr) throw std::bad_alloc();
		std::memcpy( newptr, ptr_, size_);
		if (m_allocsize) std::free( m_ptr);

		m_ptr = (char*)newptr;
		m_allocsize = mm;
	}
	else
	{
		if (m_allocsize) std::free( m_ptr);

		m_ptr = static_cast<char*>(const_cast<void*>( ptr_));
		m_allocsize = 0;
	}
	m_size = size_;
	m_id = id_;
}

void DataBlock::init( strus::Index id_, std::size_t size_)
{
	std::size_t mm = BLOCK_ALLOC_SIZE( size_);
	void* newptr = std::malloc( mm);
	if (!newptr) throw std::bad_alloc();
	std::memset( newptr, 0, size_);
	if (m_allocsize) std::free( m_ptr);

	m_ptr = (char*)newptr;
	m_allocsize = mm;
	m_size = 0;
	m_id = id_;
}

void DataBlock::swap( DataBlock& o)
{
	std::swap( m_id, o.m_id);
	std::swap( m_ptr, o.m_ptr);
	std::swap( m_size, o.m_size);
	std::swap( m_allocsize, o.m_allocsize);
}

void DataBlock::expand( std::size_t datasize)
{
	std::size_t aa = datasize + m_size;
	std::size_t mm = 1024;
	while (mm <= aa && mm != 0) mm *= 2;
	if (!mm) throw std::bad_alloc();
	char* mp;

	if (m_allocsize)
	{
		mp = (char*)std::realloc( m_ptr, mm);
		if (!mp) throw std::bad_alloc();
	}
	else
	{
		mp = (char*)std::calloc( mm, 1);
		if (!mp) throw std::bad_alloc();
		std::memcpy( mp, m_ptr, m_size);
	}
	m_allocsize = mm;
	m_ptr = mp;
}

void DataBlock::append( const void* data, std::size_t datasize)
{
	if (datasize + m_size > m_allocsize)
	{
		expand( datasize);
	}
	std::memcpy( m_ptr+m_size, data, datasize);
	m_size += datasize;
}

void DataBlock::fill( char ch, std::size_t datasize)
{
	if (datasize + m_size > m_allocsize)
	{
		expand( datasize);
	}
	std::memset( m_ptr+m_size, ch, datasize);
	m_size += datasize;
}


