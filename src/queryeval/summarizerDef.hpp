/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_DEFINITION_HPP_INCLUDED
#define _STRUS_SUMMARIZER_DEFINITION_HPP_INCLUDED
#include "strus/numericVariant.hpp"
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
			const std::string& functionName_,
			const Reference<SummarizerFunctionInstanceInterface>& function_,
			const std::vector<FeatureParameter>& featureParameters_)
		:m_function(function_)
		,m_functionName(functionName_)
		,m_featureParameters(featureParameters_){}

	SummarizerDef( const SummarizerDef& o)
		:m_function(o.m_function)
		,m_functionName(o.m_functionName)
		,m_featureParameters(o.m_featureParameters){}

	const SummarizerFunctionInstanceInterface* function() const	{return m_function.get();}
	const std::string& functionName() const				{return m_functionName;}
	const std::vector<FeatureParameter>& featureParameters() const	{return m_featureParameters;}

private:
	Reference<SummarizerFunctionInstanceInterface> m_function;	///< summarization function
	std::string m_functionName;					///< name of the summarization function
	std::vector<FeatureParameter> m_featureParameters;		///< list of feature parameters that are subject of summarization
};

}
#endif

