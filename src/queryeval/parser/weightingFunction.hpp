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
#ifndef _STRUS_QUERY_PARSER_WEIGHTING_FUNCTION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_WEIGHTING_FUNCTION_HPP_INCLUDED
#include "parser/stringIndexMap.hpp"
#include <string>
#include <vector>
#include <ostream>

namespace strus {
namespace parser {

class WeightingFunction
{
public:
	WeightingFunction()
		:m_setIndex(0),m_factor(0.0){}
	WeightingFunction( const WeightingFunction& o)
		:m_function(o.m_function),m_params(o.m_params),m_setIndex(o.m_setIndex),m_factor(o.m_factor){}
	WeightingFunction( const std::string& function_, const std::vector<float>& params_, unsigned int setIndex_, float factor_)
		:m_function(function_),m_params(params_),m_setIndex(setIndex_),m_factor(factor_){}

	static std::vector<WeightingFunction> parseExpression(
			char const*& src,
			StringIndexMap& setmap);

	static void printExpression(
			std::ostream& out, 
			std::vector<WeightingFunction> args,
			const StringIndexMap& setmap);

public:
	const std::string& function() const		{return m_function;}
	const std::vector<float>& params() const	{return m_params;}
	int setIndex() const				{return m_setIndex;}
	float factor() const				{return m_factor;}

private:
	std::string m_function;		///< specifies operation on iterator to create an accumulator. if empty then argument references an accumulator directly
	std::vector<float> m_params;	///< specifies parametrization of the weighting function
	int m_setIndex;			///< argument set index
	float m_factor;			///< multiplication factor of a calculated weight
};

}}//namespace
#endif

