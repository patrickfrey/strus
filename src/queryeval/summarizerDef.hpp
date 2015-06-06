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
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/reference.hpp"
#include <string>
#include <vector>

namespace strus {

class SummarizerDef
{
public:
	typedef QueryEvalInterface::FeatureParameter FeatureParameter;
	SummarizerDef(
			const std::string& resultAttribute_,
			const std::string& functionName_,
			SummarizerFunctionInstanceInterface* function_,
			const std::vector<FeatureParameter>& featureParameters_)
		:m_function(function_)
		,m_functionName(functionName_)
		,m_resultAttribute(resultAttribute_)
		,m_featureParameters(featureParameters_){}

	SummarizerDef( const SummarizerDef& o)
		:m_function(o.m_function)
		,m_functionName(o.m_functionName)
		,m_resultAttribute(o.m_resultAttribute)
		,m_featureParameters(o.m_featureParameters){}

	const SummarizerFunctionInstanceInterface* function() const	{return m_function.get();}
	const std::string& functionName() const				{return m_functionName;}
	const std::string& resultAttribute() const			{return m_resultAttribute;}
	const std::vector<FeatureParameter>& featureParameters() const	{return m_featureParameters;}

private:
	Reference<SummarizerFunctionInstanceInterface> m_function;	///< summarization function
	std::string m_functionName;					///< name of the summarization function
	std::string m_resultAttribute;					///< name of the result attribute the summarization is returned as
	std::vector<FeatureParameter> m_featureParameters;		///< list of feature parameters that are subject of summarization
};

}
#endif

