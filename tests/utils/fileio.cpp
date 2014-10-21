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
#include "utils/fileio.hpp"
#include "dll_tags.hpp"
#include <cstdio>
#include <cerrno>

using namespace strus;

DLL_PUBLIC unsigned int strus::readFile( const std::string& filename, std::string& res)
{
	FILE* fh = ::fopen( filename.c_str(), "rb");
	if (!fh)
	{
		return errno;
	}
	unsigned char ch;
	while (1 == ::fread( &ch, 1, 1, fh))
	{
		try
		{
			res.push_back( ch);
		}
		catch (const std::bad_alloc&)
		{
			::fclose( fh);
			return 12/*ENOMEM*/;
		}
	}
	if (!feof( fh))
	{
		unsigned int ec = ::ferror( fh);
		::fclose( fh);
		return ec;
	}
	return 0;
}

