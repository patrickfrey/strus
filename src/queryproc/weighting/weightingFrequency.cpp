/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingFrequency.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("frequency")

void WeightingFunctionContextTermFrequency::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		double weight_,
		const TermStatistics&)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			m_featar.push_back( Feature( itr_, weight_));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error creating instance of the weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextTermFrequency::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for the function '%s'"), THIS_METHOD_NAME);
}

const std::vector<WeightedField>& WeightingFunctionContextTermFrequency::call( const Index& docno)
{
	try
	{
		m_lastResult.resize( 0);
		double ww = 0.0;
		std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
		for (;fi != fe; ++fi)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				ww += fi->weight * fi->itr->frequency();
			}
		}
		m_lastResult.resize( 1);
		m_lastResult[0].setWeight( ww);
		return m_lastResult;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, m_lastResult);
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceTermFrequency::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		addNumericParameter( name_, parameterValue( name_, value));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceTermFrequency::addNumericParameter( const std::string& name_, const NumericVariant&)
{
	if (strus::caseInsensitiveEquals( name_, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

WeightingFunctionContextInterface* WeightingFunctionInstanceTermFrequency::createFunctionContext(
		const StorageClientInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new WeightingFunctionContextTermFrequency( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceTermFrequency::view() const
{
	try
	{
		return StructView();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionTermFrequency::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceTermFrequency( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionTermFrequency::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the weight of a document as sum of the feature frequency of a feature multiplied with the feature weight"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Numeric, "weight", _TXT( "defines the query feature weight factor"), "0:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* WeightingFunctionTermFrequency::name() const
{
	return THIS_METHOD_NAME;
}
const char* WeightingFunctionInstanceTermFrequency::name() const
{
	return THIS_METHOD_NAME;
}
