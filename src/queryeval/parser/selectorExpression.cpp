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
#include "parser/selectorExpression.hpp"
#include "parser/lexems.hpp"

using namespace strus;
using namespace strus::parser;

int SelectorExpression::parse( char const*& src, std::vector<SelectorExpression>& expressions, StringIndexMap& setindexmap)
{
	SelectorExpression expr;
	if (isAlpha(*src))
	{
		int setidx = setindexmap.get( parse_IDENTIFIER( src));
		expr.m_args.push_back( Argument( Argument::SetReference, setidx));
		expressions.push_back( expr);
		return expressions.size();
	}
	if (isOpenSquareBracket( *src))
	{
		parse_OPERATOR(src);
	}
	else
	{
		throw std::runtime_error( "open square bracket '[' expected for selector expression arguments");
	}
	if (isOpenOvalBracket( *src))
	{
		parse_OPERATOR(src);
		std::string funcname = parse_IDENTIFIER( src);
		if (isEqual( funcname, functionIdName( PermutationFunc)))
		{
			expr.m_functionid = PermutationFunc;
		}
		else if (isEqual( funcname, functionIdName( AscendingFunc)))
		{
			expr.m_functionid = AscendingFunc;
		}
		else if (isEqual( funcname, functionIdName( SequenceFunc)))
		{
			expr.m_functionid = SequenceFunc;
		}
		else if (isEqual( funcname, functionIdName( DistinctFunc)))
		{
			expr.m_functionid = DistinctFunc;
		}
		else if (isEqual( funcname, functionIdName( JoinFunc)))
		{
			expr.m_functionid = JoinFunc;
		}
		else if (isEqual( funcname, functionIdName( ProductFunc)))
		{
			expr.m_functionid = ProductFunc;
		}
		else
		{
			throw std::runtime_error( std::string( "unexpected operator id in selector expression: '") + funcname + "', should be one of ('D','J','A','P')");
		}
		if (isDigit( *src))
		{
			expr.m_dim = (int)parse_UNSIGNED( src);
			if (expr.m_dim < 0) throw std::runtime_error( "dimension in selector expression operator out of range");
		}
		if (isCloseOvalBracket( *src))
		{
			parse_OPERATOR( src);
		}
		else
		{
			throw std::runtime_error( "close oval bracket expected to terminate selector expression operator");
		}
	}
	while (!isCloseSquareBracket( *src))
	{
		if (isOpenSquareBracket( *src))
		{
			int subexpr = SelectorExpression::parse( src, expressions, setindexmap);
			expr.m_args.push_back( Argument( Argument::SubExpression, subexpr));
		}
		else if (isAlpha( *src))
		{
			int setidx = setindexmap.get( parse_IDENTIFIER( src));
			expr.m_args.push_back( Argument( Argument::SetReference, setidx));
		}
		else
		{
			throw std::runtime_error( "identifier or sub selector expression expected");
		}
	}
	parse_OPERATOR(src);
	expressions.push_back( expr);
	return expressions.size();
}


void SelectorExpression::print( std::ostream& out, int expridx, const std::vector<SelectorExpression>& expressions, const StringIndexMap& setindexmap)
{
	const SelectorExpression& expr = expressions[ expridx-1];
	out << "[";
	if (expr.functionid() != ProductFunc || expr.dim() >= 0)
	{
		out << "(" << functionIdName( expr.functionid());
		if (expr.dim() >= 0)
		{
			out << " " << expr.dim();
		}
		out << ") ";
	}
	std::vector<Argument>::const_iterator ai = expr.args().begin(), ae = expr.args().end();
	for (unsigned int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx)
		{
			out << " ";
		}
		switch (ai->type())
		{
			case Argument::SetReference:
				out << setindexmap.name( ai->idx());
				break;
			case Argument::SubExpression:
				SelectorExpression::print( out, ai->idx(), expressions, setindexmap);
				break;
		}
	}
	out << "]";
}

