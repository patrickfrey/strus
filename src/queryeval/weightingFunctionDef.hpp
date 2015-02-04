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
#ifndef _STRUS_QUERY_PROGRAM_WEIGHTING_FUNCTION_DEF_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_WEIGHTING_FUNCTION_DEF_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class WeightingFunctionInterface;

struct WeightingFunctionDef
{
	WeightingFunctionDef()
		:function(0),functionName(),parameters(),weightingSets(),selectorSets(){}
	WeightingFunctionDef( const WeightingFunctionDef& o)
		:function(o.function),functionName(o.functionName),parameters(o.parameters),weightingSets(o.weightingSets),selectorSets(o.selectorSets){}
	WeightingFunctionDef(
			const WeightingFunctionInterface* function_,
			const std::string& functionName_,
			const std::vector<ArithmeticVariant>& parameters_,
			const std::vector<std::string>& weightingSets_,
			const std::vector<std::string>& selectorSets_)
		:function(function_),functionName(functionName_),parameters(parameters_),weightingSets(weightingSets_),selectorSets(selectorSets_){}

	const WeightingFunctionInterface* function;	///< function used for weighting
	std::string functionName;			///< name of the function used for weighting
	std::vector<ArithmeticVariant> parameters;	///< weighting function parameters
	std::vector<std::string> weightingSets;		///< posting sets that are used for weighting
	std::vector<std::string> selectorSets;		///< posting sets selecting the documents to match
};

}
#endif

