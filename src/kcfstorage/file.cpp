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
#include <boost/lexical_cast.hpp>

#ifdef _MSC_VER
#define FILE_PATH_DELIMITER '\\'
#else
#define FILE_PATH_DELIMITER '/'
#endif
#define SET_DEFAULT_DBPATH(PATH)\
static const char* default_dbpath = "" #PATH;
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
	rt.push_back( FILE_PATH_DELIMITER);
	rt.append( name);
	rt.push_back( '.');
	rt.append( ext);
	std::replace( rt.begin(), rt.end(), '/', FILE_PATH_DELIMITER);
	return rt;
}

std::string strus::fileerror( const std::string& msg)
{
	return msg + "(system error code " + boost::lexical_cast<std::string>((int)errno) + ")";
}

int strus::fileerrno()
{
	return errno;
}

