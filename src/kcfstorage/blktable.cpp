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
#include "blktable.hpp"
#include "file.hpp"
#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <cstring>


using namespace strus;


BlockTable::BlockTable( const std::string& type_, std::size_t blocksize_, const std::string& name_, const std::string& path_, bool writemode_)
	:m_writemode(writemode_)
	,m_blocksize(blocksize_)
	,m_lastindex(0)
	,m_filename( filepath( path_, name_, type_))
	,m_filehandle(0)
{
	std::memset( &m_controlblock, 0, sizeof(m_controlblock));
	if (m_blocksize < sizeof(m_controlblock)) throw std::logic_error("block size is too small");
}

BlockTable::~BlockTable()
{
	close();
}

void BlockTable::create( const std::string& type_, std::size_t blocksize_, const std::string& name_, const std::string& path_)
{
	std::string blkfilepath( filepath( path_, name_, type_));
	if (blocksize_ < sizeof(ControlBlock)) throw std::logic_error("block size is too small");

	FILE* fh = ::fopen( blkfilepath.c_str() , "w");
	if (!fh)
	{
		throw strus::fileerror( std::string( "failed to create block file '") + blkfilepath + "'");
	}
	char* ptr = (char*)std::calloc( blocksize_, 1);
	if (!ptr) throw std::bad_alloc();
	if (0>=fwrite( ptr, blocksize_, 1, fh))
	{
		std::free( ptr);
		throw strus::fileerror( std::string( "failed to write initial block of file '") + blkfilepath + "'");
	}
	std::free( ptr);
	fclose( fh);
}

bool BlockTable::open()
{
	if (m_filehandle) close();
	const char* modestr = m_writemode?"r+":"r";
	m_filehandle = fopen( m_filename.c_str() , modestr);
	if (!m_filehandle)
	{
		m_lasterror = strus::fileerror( std::string( "failed to open block file '") + m_filename + "' for " + (m_writemode?"writing":"reading"));
		m_lasterrno = strus::fileerrno();
		return false;
	}
	if (::fseek( m_filehandle, 0L, SEEK_END) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot determine size of block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		close();
		return false;
	}
	std::size_t filesize = ::ftell( m_filehandle);
	if (::fseek( m_filehandle, 0L, SEEK_SET) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to start block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		close();
		return false;
	}
	if (filesize % m_blocksize != 0 || filesize == 0)
	{
		m_lasterror = strus::fileerror( std::string( "invalid size of block file '") + m_filename + "'");
		m_lasterrno = 0;
		close();
		return false;
	}
	m_lastindex = filesize / m_blocksize;
	if (m_writemode)
	{
		if (!readControlBlock( 0, m_controlblock))
		{
			close();
			return false;
		}
	}
	return true;
}

bool BlockTable::reset()
{
	close();
	FILE* fh = ::fopen( m_filename.c_str() , "w");
	if (!fh)
	{
		m_lasterror = strus::fileerror( std::string( "failed to create block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	char* ptr = (char*)std::calloc( m_blocksize, 1);
	if (!ptr) throw std::bad_alloc();
	if (0>=fwrite( ptr, m_blocksize, 1, fh))
	{
		std::free( ptr);
		m_lasterror = strus::fileerror( std::string( "failed to write initial block of file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	std::free( ptr);
	fclose( fh);
	open();
	return true;
}

void BlockTable::close()
{
	if (m_filehandle)
	{
		fclose( m_filehandle);
		m_filehandle = 0;
	}
}

const std::string& BlockTable::lastError() const
{
	return m_lasterror;
}

int BlockTable::lastErrno() const
{
	return m_lasterrno;
}

bool BlockTable::readBlock( Index idx, void* buf)
{
	if (::fseek( m_filehandle, idx * m_blocksize, SEEK_SET) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to read position in block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	if (0==::fread( (char*)buf, m_blocksize, 1, m_filehandle))
	{
		m_lasterror = strus::fileerror( "failed to read block from file");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	return true;
}

bool BlockTable::writeBlock( Index idx, const void* buf)
{
	if (::fseek( m_filehandle, idx * m_blocksize, SEEK_SET) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to write position in block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	if (0==::fwrite( (const char*)buf, m_blocksize, 1, m_filehandle))
	{
		m_lasterror = strus::fileerror( "failed to write block to file");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	return true;
}

Index BlockTable::appendBlock( const void* buf)
{
	if (::fseek( m_filehandle, 0, SEEK_END) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to write position at end of block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return 0;
	}
	if (0==::fwrite( (const char*)buf, m_blocksize, 1, m_filehandle))
	{
		m_lasterror = strus::fileerror( "failed to write block to file");
		m_lasterrno = strus::fileerrno();
		return 0;
	}
	return m_lastindex++;
}

bool BlockTable::readControlBlock( Index idx, ControlBlock& block)
{
	if (::fseek( m_filehandle, idx * m_blocksize, SEEK_SET) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to read position in block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	if (0==::fread( (char*)&block, sizeof(block), 1, m_filehandle))
	{
		m_lasterror = strus::fileerror( "failed to read control block from file");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	return true;
}

bool BlockTable::writeControlBlock( Index idx, const ControlBlock& block)
{
	if (::fseek( m_filehandle, idx * m_blocksize, SEEK_SET) != 0)
	{
		m_lasterror = strus::fileerror( std::string( "cannot move to write position in block file '") + m_filename + "'");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	if (0==::fwrite( (const char*)&block, sizeof(block), 1, m_filehandle))
	{
		m_lasterror = strus::fileerror( "failed to write control block to file");
		m_lasterrno = strus::fileerrno();
		return false;
	}
	return true;
}

Index BlockTable::insertBlock( const void* buf)
{
	if (m_controlblock.freelist)
	{
		ControlBlock nextblock;
		if (!readControlBlock( m_controlblock.freelist, nextblock))
		{
			return 0;
		}
		Index idx = m_controlblock.freelist;
		m_controlblock.freelist = nextblock.freelist;
		if (!writeControlBlock( 0, m_controlblock))
		{
			return 0;
		}
		if (!writeBlock( idx, buf))
		{
			m_controlblock.freelist = idx;
			writeControlBlock( 0, nextblock);
			return 0;
		}
		return idx;
	}
	else
	{
		return appendBlock( buf);
	}
}

