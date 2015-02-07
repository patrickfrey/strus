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
#ifndef _STRUS_SUMMARIZER_CONFIG_HPP_INCLUDED
#define _STRUS_SUMMARIZER_CONFIG_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include "strus/summarizerConfigInterface.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class QueryEval;

class SummarizerConfig
	:public SummarizerConfigInterface
{
public:
	SummarizerConfig(
			QueryEval* qeval_,
			const SummarizerFunctionInterface* function_,
			const std::string& functionName_);

	virtual void defineNumericParameter( const std::string& name_, const ArithmeticVariant& value_);
	virtual void defineTextualParameter( const std::string& name_, const std::string& value_);
	virtual void defineFeatureParameter( const std::string& class_, const std::string& set_);
	virtual void done();

	struct Feature
	{
		Feature( std::size_t classidx_, const std::string& set_)
			:classidx(classidx_),set(set_){}
		Feature( const Feature& o)
			:classidx(o.classidx),set(o.set){}

		std::size_t classidx;
		std::string set;
	};

	SummarizerConfig(
			QueryEval* qeval_,
			const SummarizerFunctionInterface* function_,
			const std::string& functionName_,
			const std::vector<ArithmeticVariant>& numericParameters_,
			const std::vector<std::string>& textualParameters_,
			const std::vector<Feature>& featureParameters_,
			const std::string& resultAttribute_)
		:m_qeval(qeval_)
		,m_function(function_)
		,m_functionName(functionName_)
		,m_numericParameters(numericParameters_)
		,m_textualParameters(textualParameters_)
		,m_featureParameters(featureParameters_)
		,m_resultAttribute(resultAttribute_){}

	SummarizerConfig( const SummarizerConfig& o)
		:m_qeval(o.m_qeval)
		,m_function(o.m_function)
		,m_functionName(o.m_functionName)
		,m_numericParameters(o.m_numericParameters)
		,m_textualParameters(o.m_textualParameters)
		,m_featureParameters(o.m_featureParameters)
		,m_resultAttribute(o.m_resultAttribute){}

	const SummarizerFunctionInterface* function() const			{return m_function;}
	const std::string& functionName() const					{return m_functionName;}
	const std::vector<ArithmeticVariant>& numericParameters() const		{return m_numericParameters;}
	const std::vector<std::string>& textualParameters() const		{return m_textualParameters;}
	const std::vector<Feature>& featureParameters() const			{return m_featureParameters;}
	const std::string& resultAttribute() const				{return m_resultAttribute;}

private:
	QueryEval* m_qeval;					///< query evaluation program handle
	const SummarizerFunctionInterface* m_function;		///< summarization function
	std::string m_functionName;				///< name of the summarization function
	std::vector<ArithmeticVariant> m_numericParameters;	///< summarization function numeric parameters
	std::vector<std::string> m_textualParameters;		///< summarization function textual parameters
	std::vector<Feature> m_featureParameters;		///< summarization function feature parameters
	std::string m_resultAttribute;				///< name of the result attribute the summarization is returned as
};

}
#endif

