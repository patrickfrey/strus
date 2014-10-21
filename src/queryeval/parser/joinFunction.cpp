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
#include "parser/joinFunction.hpp"
#include "parser/lexems.hpp"

using namespace strus;
using namespace strus::parser;

unsigned int JoinFunction::parse( char const*& src, std::vector<JoinFunction>& functions)
{
	JoinFunction func;
	if (isAlpha(*src))
	{
		func.m_name = parse_IDENTIFIER( src);
	}
	else
	{
		throw std::runtime_error( "identifier expected (name of function)");
	}
	if (isOpenSquareBracket( *src))
	{
		parse_OPERATOR(src);
		if (!isDigit( *src))
		{
			throw std::runtime_error( "integer number expected for range in square brackets '[' ']' after function name");
		}
		func.m_range = parse_INTEGER( src);
		if (!isCloseSquareBracket( *src))
		{
			throw std::runtime_error( "close square bracket ']' expected after range");
		}
		parse_OPERATOR(src);
	}
	if (!isOpenOvalBracket( *src))
	{
		throw std::runtime_error( "open oval bracket '(' expected for function arguments");
	}
	parse_OPERATOR(src);

	if (isCloseOvalBracket( *src))
	{
		parse_OPERATOR(src);
		functions.push_back( func);
		return functions.size();
	}
	do
	{
		if (*src == '_')
		{
			++src;
			unsigned int variable = parse_UNSIGNED1( src);
			func.m_args.push_back( Argument( Argument::Variable, variable));
		}
		else
		{
			parse( src, functions);
			func.m_args.push_back( Argument( Argument::SubExpression, functions.size()));
		}
		if (isCloseOvalBracket( *src))
		{
			parse_OPERATOR(src);
			functions.push_back( func);
			return functions.size();
		}
	}
	while (isComma( *src));
	throw std::runtime_error( "expected comma as function parameter separator or close oval bracket ')' to terminate function argument list");
}

