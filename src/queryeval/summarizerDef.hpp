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
#ifndef _STRUS_SUMMARIZER_DEFINITION_HPP_INCLUDED
#define _STRUS_SUMMARIZER_DEFINITION_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/summarizerConfig.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class QueryEval;

class SummarizerDef
{
public:
	SummarizerDef(
			const std::string& resultAttribute_,
			const SummarizerFunctionInterface* function_,
			const std::string& functionName_,
			const SummarizerConfig& config);

	SummarizerDef( const SummarizerDef& o)
		:m_function(o.m_function)
		,m_functionName(o.m_functionName)
		,m_numericParameters(o.m_numericParameters)
		,m_textualParameters(o.m_textualParameters)
		,m_featureParameters(o.m_featureParameters)
		,m_resultAttribute(o.m_resultAttribute){}

	struct Feature
	{
		Feature( std::size_t classidx_, const std::string& set_)
			:classidx(classidx_),set(set_){}
		Feature( const Feature& o)
			:classidx(o.classidx),set(o.set){}

		std::size_t classidx;
		std::string set;
	};

	const SummarizerFunctionInterface* function() const			{return m_function;}
	const std::string& functionName() const					{return m_functionName;}
	const std::vector<ArithmeticVariant>& numericParameters() const		{return m_numericParameters;}
	const std::vector<std::string>& textualParameters() const		{return m_textualParameters;}
	const std::vector<Feature>& featureParameters() const			{return m_featureParameters;}
	const std::string& resultAttribute() const				{return m_resultAttribute;}

private:
	const SummarizerFunctionInterface* m_function;		///< summarization function
	std::string m_functionName;				///< name of the summarization function
	std::vector<ArithmeticVariant> m_numericParameters;	///< summarization function numeric parameters
	std::vector<std::string> m_textualParameters;		///< summarization function textual parameters
	std::vector<Feature> m_featureParameters;		///< summarization function feature parameters
	std::string m_resultAttribute;				///< name of the result attribute the summarization is returned as
};

}
#endif

