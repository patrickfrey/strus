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
#ifndef _STRUS_KCF_FILE_HPP_INCLUDED
#define _STRUS_KCF_FILE_HPP_INCLUDED
#include <string>
#include <stdexcept>

namespace strus
{
	std::string filepath( const std::string& path, const std::string& name, const std::string& ext);
	std::string fileerror( const std::string& msg);
	int fileerrno();

	class File
	{
	public:
		File( const std::string& path_);
		bool create();
		bool open( bool write_=true);
		void close();
		std::size_t size();
		bool seek( std::size_t pos_);
		bool write( void* buf, std::size_t bufsize);
		bool read( void* buf, std::size_t bufsize);

		const std::string& error() const	{return m_error;}
		int errno() const			{return m_errno;}

	private:
		int m_fd;
		std::string m_path;
		std::string m_error;
		int m_errno;
	};
}

#endif


