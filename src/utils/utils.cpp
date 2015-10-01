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
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <unistd.h>

using namespace strus;
using namespace strus::utils;

std::string utils::tolower( const std::string& val)
{
	return boost::algorithm::to_lower_copy( val);
}

std::string utils::trim( const std::string& val)
{
	return boost::algorithm::trim_copy( val);
}

bool utils::caseInsensitiveEquals( const std::string& val1, const std::string& val2)
{
	return boost::algorithm::iequals( val1, val2);
}

bool utils::caseInsensitiveStartsWith( const std::string& val, const std::string& prefix)
{
	return boost::algorithm::istarts_with( val, prefix);
}

int utils::toint( const std::string& val)
{
	try
	{
		return boost::lexical_cast<int>( val);
	}
	catch (const boost::bad_lexical_cast& err)
	{
		throw strus::runtime_error( _TXT( "failed to convert string '%s' to integer: %s"), val.c_str(), err.what());
	}
}

std::string utils::tostring( int val)
{
	try
	{
		return boost::lexical_cast<std::string>( val);
	}
	catch (...)
	{
		throw strus::runtime_error( _TXT( "failed to convert number to string (out of memory)"));
	}
}


///\brief Implementation of alligned malloc copied from http://jmabille.github.io/blog/2014/12/06/aligned-memory-allocator/
#define HAS_POSIX_MEMALIGN (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
void* utils::aligned_malloc( std::size_t size, std::size_t alignment)
{
#if HAS_POSIX_MEMALIGN
	void* res;
	const int failed = posix_memalign( &res, size, alignment);
	if (failed) res = 0;
	return res;
#elif (defined _MSC_VER)
	return _aligned_malloc( size, alignment);
#else
	void* res = 0;
	void* ptr = std::malloc( size + alignment);
	if(ptr != 0)
	{
		res = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(ptr) & ~(std::size_t(alignment-1))) + alignment);
		*(reinterpret_cast<void**>(res) - 1) = ptr;
	}
	return res;
#endif
}

void utils::aligned_free( void *ptr)
{
#if HAS_POSIX_MEMALIGN
	std::free( ptr);
#elif (defined _MSC_VER)
	return _aligned_free( ptr);
#else
	std::free( *(reinterpret_cast<void**>(ptr) - 1));
#endif
}

