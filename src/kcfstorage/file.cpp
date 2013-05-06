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
#include "file.hpp"
#include <cstdio>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <boost/lexical_cast.hpp>

#ifdef _MSC_VER
#define FILE_PATH_DELIMITER '\\'
#else
#define FILE_PATH_DELIMITER '/'
#endif
#define SET_DEFAULT_DBPATH2(PATH)\
static const char* default_dbpath = "" #PATH;
#define SET_DEFAULT_DBPATH(PATH) SET_DEFAULT_DBPATH2(PATH)
SET_DEFAULT_DBPATH(STRUS_DEFAULT_DBPATH)

using namespace strus;

std::string strus::filepath( const std::string& path, const std::string& name, const std::string& ext)
{
	std::string rt;
	if (path.empty())
	{
		rt.append( default_dbpath);
	}
	else
	{
		rt.append( path);
	}
	if (!rt.empty() && rt.at(rt.size()-1) != FILE_PATH_DELIMITER)
	{
		rt.push_back( FILE_PATH_DELIMITER);
	}
	rt.append( name);
	rt.push_back( '.');
	rt.append( ext);
	std::replace( rt.begin(), rt.end(), '/', FILE_PATH_DELIMITER);
	return rt;
}

File::File( const std::string& path_)
	:m_fd(-1),m_path(path_)
{
}

File::~File()
{
	close();
}

static std::string errorstr( const std::string& path, const std::string& msg, int errno_)
{
	std::string rt;
	rt.append( ::strerror( errno_));
	rt.append( " ");
	rt.append( msg);
	rt.append( " '");
	rt.append( path);
	rt.append( "'");
	return rt;
}

void File::create()
{
	if (m_fd >= 0) ::close(m_fd);
	m_fd = ::open( m_path.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0644);
	if (m_fd < 0)
	{
		throw std::runtime_error( errorstr( m_path, "creating file", errno));
	}
}

void File::open( bool write_)
{
	if (m_fd >= 0) ::close(m_fd);
	int flags = (write_)?O_RDWR:O_RDONLY;
	m_fd = ::open( m_path.c_str(), flags, 0644);
	if (m_fd < 0)
	{
		const char* msg = (write_)?"opening file for writing":"opening file for reading";
		throw std::runtime_error( errorstr( m_path, msg, errno));
	}
}

void File::close()
{
	if (m_fd >= 0) ::close(m_fd);
	m_fd = -1;
}

void File::seek( std::size_t pos_)
{
	if (::lseek( m_fd, pos_, SEEK_SET) < 0)
	{
		throw std::runtime_error( errorstr( m_path, "seeking file position", errno));
	}
}

std::size_t File::seek_end()
{
	::ssize_t rt = ::lseek( m_fd, 0, SEEK_END);
	if (rt < 0)
	{
		throw std::runtime_error( errorstr( m_path, "seeking file position (end of file)", errno));
	}
	return (std::size_t)rt;
}

std::size_t File::position()
{
	::ssize_t rt = ::lseek( m_fd, 0, SEEK_CUR);
	if (rt < 0)
	{
		throw std::runtime_error( errorstr( m_path, "evaluating current position in file", errno));
	}
	return (std::size_t)rt;
}

std::size_t File::filesize()
{
	std::size_t pos = position();
	std::size_t rt = seek_end();
	seek( pos);
	return rt;
}

void File::write( const void* buf, std::size_t bufsize)
{
	if (::write( m_fd, buf, bufsize) < 0)
	{
		throw std::runtime_error( errorstr( m_path, "writing file", errno));
	}
}

void File::read( void* buf, std::size_t bufsize)
{
	if (::read( m_fd, buf, bufsize) < 0)
	{
		throw std::runtime_error( errorstr( m_path, "writing file", errno));
	}
}



