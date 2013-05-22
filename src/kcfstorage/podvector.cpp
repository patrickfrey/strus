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
#include "podvector.hpp"
#include <cstdlib>

using namespace strus;

std::size_t PodVectorBase::blocksize() const
{
	return m_elementsize * (NofBlockElements+1);
}

PodVectorBase::PodVectorBase( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_)
	:BlockTable( type_, name_, path_, m_elementsize * (NofBlockElements+1))
	,m_lastidx(0)
	,m_elementsize(elementsize_)
{
	if (m_elementsize < sizeof( std::size_t)) throw std::logic_error("element size too small");
	m_cur = (char*)std::calloc( 1, blocksize());
	if (!m_cur) throw std::bad_alloc();
}

PodVectorBase::~PodVectorBase()
{
	std::free( m_cur);
}

void PodVectorBase::create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t elementsize_)
{
	BlockTable::create( type_, name_, path_, elementsize_ * (NofBlockElements+1));
}

void PodVectorBase::create()
{
	BlockTable::create();
}

void PodVectorBase::close()
{
	BlockTable::close();
}

void PodVectorBase::open( bool writemode_)
{
	BlockTable::open( writemode_);
	std::size_t nof_blocks = BlockTable::size();
	if (nof_blocks > 0)
	{
		m_lastidx = nof_blocks;
		readBlock( m_lastidx, m_cur);
	}
	else
	{
		m_lastidx = 0;
		std::memset( m_cur, 0, blocksize());
	}
}

Index PodVectorBase::size() const
{
	std::size_t nn = 0;
	std::memcpy( &nn, m_cur, sizeof(nn));
	return nn + (NofBlockElements * m_lastidx);
}

Index PodVectorBase::push_back( const void* element)
{
	Index rt = 0;
	std::size_t nn = 0;
	std::memcpy( &nn, m_cur, sizeof(nn));

	if (m_lastidx == 0 || nn == NofBlockElements)
	{
		std::size_t nn = 1;
		char* newblk = (char*)std::calloc( 1, blocksize());
		if (!newblk) throw std::bad_alloc();
		std::memcpy( newblk, &nn, sizeof(nn));
		std::memcpy( newblk + m_elementsize, element, m_elementsize);
		try
		{
			m_lastidx = BlockTable::insertBlock( newblk);
			std::memcpy( m_cur, newblk, blocksize());
			std::free( newblk);
		}
		catch (const std::runtime_error& e)
		{
			std::free( newblk);
			throw e;
		}
		rt = (m_lastidx-1) * NofBlockElements;
	}
	else
	{
		rt = (m_lastidx-1) * NofBlockElements + nn;
		std::memcpy( m_cur + (m_elementsize * ++nn), element, m_elementsize);
		BlockTable::writeBlock( m_lastidx, m_cur);
		std::memcpy( m_cur, &nn, sizeof(nn));
	}
	return rt;
}

void PodVectorBase::set( const Index& idx, const void* element)
{
	BlockTable::partialWriteBlock( (idx / NofBlockElements) + 1, idx % NofBlockElements, element, m_elementsize);
}

void PodVectorBase::get( const Index& idx, void* element) const
{
	BlockTable::partialReadBlock( (idx / NofBlockElements) + 1, idx % NofBlockElements, element, m_elementsize);
}

Index PodVectorBase::fill( std::size_t nof_elements, const void* fillerelement)
{
	Index rt;
	std::size_t ii=0;
	for (ii=0; ii<nof_elements; ++ii)
	{
		rt = push_back( fillerelement);
	}
	return rt;
}

void PodVectorBase::reset()
{
	BlockTable::reset();
	std::memset( &m_cur, 0, blocksize());
	m_lastidx = 0;
}

PodVectorBase::iterator::iterator( PodVectorBase* ref_)
	:m_ref(ref_)
	,m_cur((char*)std::calloc(1,ref_->blocksize()))
	,m_curidx(0)
	,m_curpos(0)
	,m_cursize(0)
{
	if (!m_cur) throw std::bad_alloc();
	readNextBlock();
}

PodVectorBase::iterator::iterator()
	:m_ref(0),m_cur(0),m_curidx(0),m_curpos(0),m_cursize(0)
{}

PodVectorBase::iterator::iterator( const iterator& o)
	:m_ref(o.m_ref)
	,m_cur((char*)std::calloc(1,o.m_ref->blocksize()))
	,m_curidx(o.m_curidx)
	,m_curpos(o.m_curpos)
	,m_cursize(o.m_cursize)
{
	if (!m_cur) throw std::bad_alloc();
	std::memcpy( m_cur, o.m_cur, m_ref->blocksize());
}

PodVectorBase::iterator& PodVectorBase::iterator::operator++()
{
	m_curpos++;
	if (m_curpos == m_cursize) readNextBlock();
	return *this;
}

const char* PodVectorBase::iterator::operator*()
{
	if (m_curpos == m_cursize) throw std::runtime_error( "uninitialized memory read");
	return m_cur + ((m_curpos+1) * m_ref->m_elementsize);
}

bool PodVectorBase::iterator::isequal( const iterator& o) const
{
	if (!o.m_ref)
	{
		if (!m_ref) return true;
		return (m_curidx == m_ref->m_lastidx && m_curpos == m_cursize);
	}
	else if (!m_ref)
	{
		return o.isequal( *this);
	}
	return (m_curidx == o.m_curidx && m_curpos == o.m_curpos);
}

void PodVectorBase::iterator::readNextBlock()
{
	if (m_curidx == m_ref->m_lastidx) throw std::runtime_error( "try to read over list boundaries");
	m_ref->readBlock( m_curidx+1, &m_cur);
	m_curpos = 0;
	std::memcpy( &m_cursize, m_cur, sizeof(std::size_t));
	++m_curidx;
}

