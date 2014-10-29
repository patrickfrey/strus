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
#ifndef _STRUS_QUERY_PARSER_LEXEMS_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_LEXEMS_HPP_INCLUDED
#include <string>

namespace strus {
namespace parser {

static inline bool isAlpha( char ch)
{
	return ((ch|32) <= 'z' && (ch|32) >= 'a') || (ch) == '_';
}
static inline bool isDigit( char ch)
{
	return (ch <= '9' && ch >= '0');
}
static inline bool isMinus( char ch)
{
	return (ch == '-');
}
static inline bool isAlnum( char ch)
{
	return isAlpha(ch) || isDigit(ch);
}
static inline bool isAssign( char ch)
{
	return ch == '=';
}
static inline bool isColon( char ch)
{
	return ch == ':';
}
static inline bool isSemiColon( char ch)
{
	return ch == ';';
}
static inline bool isDot( char ch)
{
	return ch == '.';
}
static inline bool isComma( char ch)
{
	return ch == ',';
}
static inline bool isOpenSquareBracket( char ch)
{
	return ch == '[';
}
static inline bool isCloseSquareBracket( char ch)
{
	return ch == ']';
}
static inline bool isOpenOvalBracket( char ch)
{
	return ch == '(';
}
static inline bool isCloseOvalBracket( char ch)
{
	return ch == ')';
}
static inline bool isOpenAngleBracket( char ch)
{
	return ch == '<';
}
static inline bool isCloseAngleBracket( char ch)
{
	return ch == '>';
}
static inline bool isAsterisk( char ch)
{
	return ch == '*';
}
static inline bool isStringQuote( char ch)
{
	return ch == '\'' || ch == '"';
}
static inline bool isSpace( char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}
static inline void skipToEoln( char const*& src)
{
	while (*src && *src != '\n') ++src;
}
static inline void skipSpaces( char const*& src)
{
	for (;;)
	{
		while (isSpace( *src)) ++src;
		if (*src == '#')
		{
			++src;
			skipToEoln( src);
		}
		else
		{
			break;
		}
	}
}

bool isEqual( const std::string& id, const char* idstr);
std::string parse_IDENTIFIER( char const*& src);
std::string parse_STRING( char const*& src);
unsigned int parse_UNSIGNED( char const*& src);
unsigned int parse_UNSIGNED1( char const*& src);
float parse_FLOAT( char const*& src);
char parse_OPERATOR( char const*& src);
int parse_INTEGER( char const*& src);
int parse_KEYWORD( char const*& src, unsigned int nof, ...);
int parse_KEYWORD( unsigned int& duplicateflags, char const*& src, unsigned int nof, ...);

}}//namespace
#endif
