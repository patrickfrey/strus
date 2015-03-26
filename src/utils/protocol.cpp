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
#include "strus/private/protocol.hpp"
#include "private/dll_tags.hpp"
#include "strus/private/internationalization.hpp"
#include <stdexcept>

using namespace strus;

DLL_PUBLIC std::string Protocol::encodeString( const std::string& value)
{
	const char* hex = "0123456789ABCDEF";
	std::string rt;
	char const* vv = value.c_str();
	for (;*vv; ++vv)
	{
		if ((signed char)*vv <= 32 || *vv == '#')
		{
			rt.push_back('#');
			rt.push_back( hex[ (unsigned char)*vv >>  4]);
			rt.push_back( hex[ (unsigned char)*vv & 0xF]);
		}
		else
		{
			rt.push_back( *vv);
		}
	}
	return rt;
}

unsigned int decode_esc( char ch)
{
	ch |= 32;
	if (ch >= '0' && ch <= '9')
	{
		return ch - '0';
	}
	else if (ch >= 'a' && ch <= 'f')
	{
		return ch - 'a';
	}
	else
	{
		throw strus::runtime_error( _TXT( "illegal character in protocol encoding"));
	}
}

DLL_PUBLIC std::string Protocol::decodeString( const std::string& value)
{
	std::string rt;
	char const* vv = value.c_str();
	for (; *vv; ++vv)
	{
		if (*vv == '#')
		{
			unsigned int chr = decode_esc( vv[1]) + decode_esc( vv[2]);
			rt.push_back( (unsigned char)chr);
			vv += 2;
		}
		else
		{
			rt.push_back( *vv);
		}
	}
	return rt;
}


