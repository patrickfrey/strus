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
#include "weightingFunction.hpp"
#include "parser/lexems.hpp"

using namespace strus;
using namespace strus::parser;

std::vector<WeightingFunction> WeightingFunction::parseExpression(
		char const*& src,
		KeyMap<SetDimDescription>& setmap)
{
	std::vector<WeightingFunction> rt;
	if (isCloseOvalBracket(*src))
	{
		OPERATOR(src);
		return rt;
	}
	for (;;)
	{
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "accumulator identifier expected");
		}
		std::string function( IDENTIFIER( src));
		int argid = 0;
		std::vector<float> params;
		float factor = 1.0;

		if (!isOpenAngleBracket(*src))
		{
			throw std::runtime_error("expected accumulator summand in angle brackets '<','>'");
		}
		OPERATOR(src);
		if (!isAlpha( *src)) throw std::runtime_error("expected identifier (occurrence set) in angle brackets as operand of an accumulate operation");

		std::string setname( IDENTIFIER( src));

		KeyMap<SetDimDescription>::iterator si = setmap.find( setname);
		if (si != setmap.end())
		{
			si->second.referenced = true;
			argid = si->second.id;
		}
		while (isComma(*src))
		{
			OPERATOR(src);
			if (!isDigit(*src))
			{
				throw std::runtime_error("numeric scalar argument expected in accumulation operation");
			}
			params.push_back( FLOAT( src));
		}
		if (!isCloseAngleBracket(*src)) throw std::runtime_error("expected close angle bracket to close declaration of occurrency accumulator");
		OPERATOR(src);

		if (isAsterisk( *src))
		{
			OPERATOR( src);
			if (isDigit( *src))
			{
				factor = FLOAT( src);
			}
			else
			{
				throw std::runtime_error("floating point number expected as weight after '*'");
			}
		}
		if (argid != 0)
		{
			rt.push_back( WeightingFunction( function, params, argid, factor));
		}
		if (isComma( *src))
		{
			OPERATOR(src);
		}
		else if (isCloseOvalBracket(*src))
		{
			OPERATOR(src);
			return rt;
		}
		else
		{
			throw std::runtime_error("comma ',' as argument separator or close oval bracket ')' to terminate argument list expected");
		}
	}
	return rt;
}

