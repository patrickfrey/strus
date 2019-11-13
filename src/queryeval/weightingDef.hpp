/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_WEIGHTING_DEFINITION_HPP_INCLUDED
#define _STRUS_WEIGHTING_DEFINITION_HPP_INCLUDED
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class QueryEval;

class WeightingDef
{
public:
	typedef QueryEvalInterface::FeatureParameter FeatureParameter;

	WeightingDef()
		:m_function(),m_featureParameters(),m_debugAttributeName(){}

	WeightingDef( const WeightingDef& o)
		:m_function(o.m_function)
		,m_featureParameters(o.m_featureParameters)
		,m_debugAttributeName(o.m_debugAttributeName){}

	WeightingDef(
			const Reference<WeightingFunctionInstanceInterface>& function_,
			const std::vector<FeatureParameter>& featureParameters_,
			const std::string& debugAttributeName_)
		:m_function(function_)
		,m_featureParameters(featureParameters_)
		,m_debugAttributeName(debugAttributeName_){}

	const WeightingFunctionInstanceInterface* function() const	{return m_function.get();}
	const std::vector<FeatureParameter>& featureParameters() const	{return m_featureParameters;}
	const std::string& debugAttributeName() const			{return m_debugAttributeName;}

private:
	Reference<WeightingFunctionInstanceInterface> m_function;	///< parameterized function used for weighting
	std::vector<FeatureParameter> m_featureParameters;		///< list of feature parameters that are subject of weighting
	std::string m_debugAttributeName;				///< attribute where debug info is attached in case of debug enabled in query
};

}
#endif

