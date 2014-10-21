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
#include <boost/shared_ptr.hpp>

namespace strus {
namespace parser {

struct WeightingFunction
{
	std::string function;		///< specifies operation on iterator to create an accumulator. if empty then argument references an accumulator directly
	std::vector<float> params;	///< specifies parametrization of the weighting function
	int setIndex;			///< argument set index
	double factor;			///< multiplication factor of a calculated weight

	WeightingFunction()
		:setIndex(0),factor(0.0){}
	WeightingFunction( const WeightingFunction& o)
		:function(o.function),params(o.params),setIndex(o.setIndex),factor(o.factor){}
	WeightingFunction( const std::string& function_, const std::vector<float>& params_, unsigned int setIndex_, double factor_=1.0)
		:function(function_),params(params_),setIndex(setIndex_),factor(factor_){}

	static std::vector<WeightingFunction> parseExpression(
			char const*& src,
			StringIndexMap& setmap);
};

}}//namespace
#endif

