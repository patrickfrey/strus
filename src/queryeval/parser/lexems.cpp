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
#include <cstdarg>
#include <sstream>
#include <iostream>
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

std::string parser::parse_IDENTIFIER( char const*& src)
{
	std::string rt;
	while (isAlnum( *src)) rt.push_back( *src++);
	skipSpaces( src);
	return rt;
}

std::string parser::parse_STRING( char const*& src)
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

unsigned int parser::parse_UNSIGNED( char const*& src)
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

unsigned int parser::parse_UNSIGNED1( char const*& src)
{
	unsigned int rt = parse_UNSIGNED( src);
	if (rt == 0) throw std::runtime_error( "positive unsigned integer expected");
	return rt;
}

float parser::parse_FLOAT( char const*& src)
{
	unsigned int digitsAllowed = 9;
	float rt = 0.0;
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

char parser::parse_OPERATOR( char const*& src)
{
	char rt = *src++;
	skipSpaces( src);
	return rt;
}

int parser::parse_INTEGER( char const*& src)
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

static int checkKeyword( std::string id, int nn, va_list argp)
{
	for (int ii=0; ii<nn; ++ii)
	{
		const char* keyword = va_arg( argp, const char*);
		if (isEqual( id, keyword))
		{
			return ii;
		}
	}
	return -1;
}

static std::string keywordList( va_list argp, int nn)
{
	std::ostringstream msg;
	for (int ii=0; ii<nn; ++ii)
	{
		const char* keyword = va_arg( argp, const char*);
		if (ii > 0) msg << " ,";
		msg << "'" << keyword << "'";
	}
	return msg.str();
}

int parser::parse_KEYWORD( char const*& src, unsigned int nn, ...)
{
	char const* src_bk = src;
	va_list argp;
	va_start( argp, nn);

	std::string id = parse_IDENTIFIER( src);
	va_start( argp, nn);

	int ii = checkKeyword( id, nn, argp);
	if (ii < 0)
	{
		src = src_bk;
		va_start( argp, nn);
		throw std::runtime_error(
			std::string( "unknown keyword '") + id
			+ "', one of " + keywordList( argp, nn) + " expected");
	}
	va_end( argp);
	return ii;
}

int parser::parse_KEYWORD( unsigned int& duplicateflags, char const*& src, unsigned int nn, ...)
{
	char const* src_bk = src;
	va_list argp;
	va_start( argp, nn);

	if (nn > sizeof(unsigned int)*8) throw std::logic_error("too many arguments (parse_KEYWORD)");

	std::string id = parse_IDENTIFIER( src);
	va_start( argp, nn);

	int ii = checkKeyword( id, nn, argp);
	if (ii < 0)
	{
		src = src_bk;
		va_start( argp, nn);
		throw std::runtime_error(
			std::string( "unknown keyword '") + id
			+ "', one of " + keywordList( argp, nn) + " expected");
	}
	va_end( argp);
	if ((duplicateflags & (1 << ii))!= 0)
	{
		throw std::runtime_error( std::string( "duplicate definition of '") + id + "'");
	}
	duplicateflags |= (1 << ii);
	return ii;
}

