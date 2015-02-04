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
#ifndef _STRUS_QUERY_PROGRAM_SUMMARIZER_DEF_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_SUMMARIZER_DEF_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;

struct SummarizerDef
{
	SummarizerDef( 
			const SummarizerFunctionInterface* function_,
			const std::string& functionName_,
			const std::vector<ArithmeticVariant>& parameters_,
			const std::string& resultAttribute_,
			const std::string& contentType_,
			const std::string& structSet_,
			const std::vector<std::string>& featureSet_)
		:function(function_)
		,functionName(functionName_)
		,parameters(parameters_)
		,resultAttribute(resultAttribute_)
		,contentType(contentType_)
		,structSet(structSet_)
		,featureSet(featureSet_){}

	SummarizerDef( const SummarizerDef& o)
		:function(o.function)
		,functionName(o.functionName)
		,parameters(o.parameters)
		,resultAttribute(o.resultAttribute)
		,contentType(o.contentType)
		,structSet(o.structSet)
		,featureSet(o.featureSet){}

	const SummarizerFunctionInterface* function;	///< summarization function
	std::string functionName;			///< name of the summarization function
	std::vector<ArithmeticVariant> parameters;	///< summarization function parameters
	std::string resultAttribute;			///< name of the result attribute the summarization is returned as
	std::string contentType;			///< content type to extract from the forward indes as result of summarization
	std::string structSet;				///< set of structure elements
	std::vector<std::string> featureSet;		///< set of features to seek for matches for summarization
};

}
#endif

