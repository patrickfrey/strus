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
#include "parser/lexems.hpp"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

using namespace strus;
using namespace strus::parser;

bool parser::isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}

std::string parser::IDENTIFIER( char const*& src)
{
	std::string rt;
	while (isAlnum( *src)) rt.push_back( *src++);
	skipSpaces( src);
	return rt;
}

std::string parser::STRING( char const*& src)
{
	std::string rt;
	char eb = *src++;
	while (*src != eb)
	{
		if (*src == '\0' || *src == '\n') throw std::runtime_error("unterminated string");
		if (*src == '\\')
		{
			src++;
			if (*src == '\0' || *src == '\n') throw std::runtime_error("unterminated string");
		}
		rt.push_back( *src++);
	}
	skipSpaces( src);
	return rt;
}

unsigned int parser::UNSIGNED( char const*& src)
{
	unsigned int rt = 0;
	while (isDigit( *src))
	{
		unsigned int vv = (rt * 10) + (*src - '0');
		if (vv <= rt) throw std::runtime_error( "index out of range");
		rt = vv;
		++src;
	}
	skipSpaces( src);
	return rt;
}

float parser::FLOAT( char const*& src)
{
	unsigned int digitsAllowed = 9;
	float rt = 1.0;
	float div = 1.0;
	if (*src == '-')
	{
		++src;
		rt = -1.0;
	}
	while (isDigit( *src) && digitsAllowed)
	{
		rt = (rt * 10.0) + (*src - '0');
		++src;
		--digitsAllowed;
	}
	if (isDot( *src))
	{
		++src;
		while (isDigit( *src) && digitsAllowed)
		{
			div /= 10.0;
			rt = (rt * 10.0) + (*src - '0');
			++src;
			--digitsAllowed;
		}
	}
	if (!digitsAllowed)
	{
		throw std::runtime_error( "floating point number out of range");
	}
	skipSpaces( src);
	return rt * div;
}

char parser::OPERATOR( char const*& src)
{
	char rt = *src++;
	skipSpaces( src);
	return rt;
}

int parser::INTEGER( char const*& src)
{
	int rt = 0;
	int prev = 0;
	if (!*src) throw std::runtime_error("integer expected");
	bool neg = false;
	if (*src == '-')
	{
		++src;
		neg = true;
	}
	if (!(*src >= '0' && *src <= '9')) throw std::runtime_error("integer expected");

	for (; *src >= '0' && *src <= '9'; ++src)
	{
		rt = (rt * 10) + (*src - '0');
		if (prev > rt) throw std::runtime_error("integer number out of range");
		prev = rt;
	}
	if (isAlpha(*src)) throw std::runtime_error("integer expected");

	skipSpaces( src);
	if (neg)
	{
		return -rt;
	}
	else
	{
		return rt;
	}
}

