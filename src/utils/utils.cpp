/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <unistd.h>
#include <stdlib.h>

using namespace strus;
using namespace strus::utils;

std::string utils::tolower( const char* val)
{
	std::size_t len = std::strlen(val)+1;
	std::string rt;
	rt.reserve( len);
	char const* vv = val;
	while (*vv) rt.push_back( std::tolower(*vv++));
	return rt;
}

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

float utils::tofloat( const std::string& val)
{
	try
	{
		return boost::lexical_cast<float>( val);
	}
	catch (const boost::bad_lexical_cast& err)
	{
		throw strus::runtime_error( _TXT( "failed to convert string '%s' to float: %s"), val.c_str(), err.what());
	}
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

float utils::tofraction( const std::string& val)
{
	std::string vv( trim( val));
	if (vv.empty()) throw strus::runtime_error(_TXT("failed to convert percentage fraction value (empty)"));
	if (vv[ vv.size()] == '%')
	{
		vv.resize( vv.size()-1);
		float rt = tofloat( val);
		return rt / 100;
	}
	else
	{
		return tofloat( val);
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


///\note Currently based on 'memalign' declared as obsolete
///	For an alternative implementation of alligned malloc see http://jmabille.github.io/blog/2014/12/06/aligned-memory-allocator/
void* utils::aligned_malloc( std::size_t size, std::size_t alignment)
{
#if (defined _MSC_VER)
	return _aligned_malloc( size, alignment);
#else
	void* rt = 0;
	return (0==posix_memalign( &rt, alignment, size))?rt:0;
#endif
}

void utils::aligned_free( void *ptr)
{
#if (defined _MSC_VER)
	return _aligned_free( ptr);
#else
	free( ptr);
#endif
}


