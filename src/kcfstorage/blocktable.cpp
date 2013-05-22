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
#include "blocktable.hpp"
#include "file.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>

using namespace strus;

BlockTable::BlockTable( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t blocksize_)
	:m_writemode(false)
	,m_blocksize(blocksize_)
	,m_file( filepath( path_, name_, type_))
{
	std::memset( &m_controlblock, 0, sizeof(m_controlblock));
	if (m_blocksize < sizeof(m_controlblock)) throw std::logic_error("block size is too small");
}

BlockTable::~BlockTable()
{
	close();
}

void BlockTable::create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t blocksize_)
{
	if (blocksize_ < sizeof(ControlBlock)) throw std::logic_error("block size is too small");
	BlockTable tab( type_, name_, path_, blocksize_);
	tab.create();
}

void BlockTable::create()
{
	m_file.create();

	char* ptr = (char*)std::calloc( m_blocksize, 1);
	if (!ptr) throw std::bad_alloc();
	try
	{
		m_file.awrite( ptr, m_blocksize);
	}
	catch (const std::runtime_error& e)
	{
		std::free( ptr);
		throw e;
	}
}

void BlockTable::open( bool writemode_)
{
	m_file.open( m_writemode = writemode_);
	std::size_t filesize = m_file.filesize();

	if (filesize % m_blocksize != 0 || filesize == 0)
	{
		throw std::runtime_error( std::string( "invalid size of block file '") + m_file.path() + "'");
	}
	if (m_writemode)
	{
		readControlBlock( 0, m_controlblock);
	}
	else
	{
		std::memset( &m_controlblock, 0, sizeof(m_controlblock));
	}
}

void BlockTable::reset()
{
	m_file.create();
	char* ptr = (char*)std::calloc( m_blocksize, 1);
	if (!ptr) throw std::bad_alloc();
	try
	{
		m_file.awrite( ptr, m_blocksize);
	}
	catch (const std::runtime_error& e)
	{
		std::free( ptr);
		throw e;
	}
	std::free( ptr);
	m_file.close();
	m_file.open( true);
}

void BlockTable::close()
{
	m_file.close();
}

void BlockTable::readBlock( Index idx, void* buf) const
{
	m_file.pread( idx * m_blocksize, (char*)buf, m_blocksize);
}

void BlockTable::writeBlock( Index idx, const void* buf)
{
	if (!m_writemode) throw std::runtime_error("illegal write operation on file opened for reading");
	m_file.pwrite( idx * m_blocksize, (char*)buf, m_blocksize);
}

void BlockTable::partialReadBlock( Index idx, std::size_t pos, void* buf, std::size_t bufsize) const
{
	if (pos + bufsize > m_blocksize) throw std::runtime_error("bad arguments for partial read operation");
	m_file.pread( idx * m_blocksize + pos, (char*)buf, bufsize);
}

void BlockTable::partialWriteBlock( Index idx, std::size_t pos, const void* buf, std::size_t bufsize)
{
	if (!m_writemode) throw std::runtime_error("illegal write operation on file opened for reading");
	if (pos + bufsize > m_blocksize) throw std::runtime_error("bad arguments for partial write operation");
	m_file.pwrite( idx * m_blocksize + pos, (char*)buf, bufsize);
}

Index BlockTable::appendBlock( const void* buf)
{
	if (!m_writemode) throw std::runtime_error("illegal write operation on file opened for reading");
	std::size_t pos = m_file.awrite( (char*)buf, m_blocksize);
	return (pos / m_blocksize);
}

Index BlockTable::size() const
{
	Index rt;
	if (!m_file.isopen())
	{
		throw std::runtime_error("called size of block table without opening it");
	}
	else
	{
		rt = (m_file.filesize() / m_blocksize);
		if (!rt) throw std::runtime_error("unknown file format (header block missing)");
	}
	return rt-1;
}

void BlockTable::readControlBlock( Index idx, ControlBlock& block)
{
	m_file.pread( idx * m_blocksize, (char*)&block, sizeof(block));
}

void BlockTable::writeControlBlock( Index idx, const ControlBlock& block)
{
	if (!m_writemode) throw std::runtime_error("illegal write operation on file opened for reading");
	m_file.pwrite( idx * m_blocksize, (char*)&block, sizeof(block));
}

Index BlockTable::insertBlock( const void* buf)
{
	if (!m_writemode) throw std::runtime_error("illegal write operation on file opened for reading");
	if (m_controlblock.freelist)
	{
		ControlBlock nextblock;
		readControlBlock( m_controlblock.freelist, nextblock);

		Index idx = m_controlblock.freelist;
		m_controlblock.freelist = nextblock.freelist;
		writeControlBlock( 0, m_controlblock);
		try
		{
			writeBlock( idx, buf);
		}
		catch (const std::runtime_error& e)
		{
			m_controlblock.freelist = idx;
			writeControlBlock( 0, m_controlblock);
			//... if this fails, we get a leak (one unreferenced block)
			throw e;
		}
		return idx;
	}
	else
	{
		return appendBlock( buf);
	}
}

