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
#ifndef _STRUS_WEIGHTING_DEFINITION_HPP_INCLUDED
#define _STRUS_WEIGHTING_DEFINITION_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/weightingConfig.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class QueryEval;

class WeightingDef
{
public:
	WeightingDef()
		:m_function(0),m_functionName(),m_parameters(){}
	WeightingDef( const WeightingDef& o)
		:m_function(o.m_function),m_functionName(o.m_functionName),m_parameters(o.m_parameters){}
	WeightingDef(
			const WeightingFunctionInterface* function_,
			const std::string& functionName_,
			const WeightingConfig& config,
			const std::vector<std::string>& weightingSets_);

	const WeightingFunctionInterface* function() const		{return m_function;}
	const std::string& functionName() const				{return m_functionName;}
	const std::vector<ArithmeticVariant>& parameters() const	{return m_parameters;}
	const std::vector<std::string>& weightingSets() const		{return m_weightingSets;}

private:
	const WeightingFunctionInterface* m_function;	///< function used for weighting
	std::string m_functionName;			///< name of the function used for weighting
	std::vector<ArithmeticVariant> m_parameters;	///< weighting function parameters
	std::vector<std::string> m_weightingSets;	///< posting sets that are used for weighting
};

}
#endif

