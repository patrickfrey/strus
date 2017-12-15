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
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define METHOD_NAME "constant"

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
			m_errorhnd->report( _TXT("unknown '%s' weighting function feature parameter '%s'"), METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error creating instance of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextTermFrequency::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( _TXT("no variables known for function '%s'"), METHOD_NAME);
}

double WeightingFunctionContextTermFrequency::call( const Index& docno)
{
	double rt = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for (;fi != fe; ++fi)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			rt += fi->weight * fi->itr->frequency();
		}
	}
	return rt;
}

std::string WeightingFunctionContextTermFrequency::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "calculate %s"), METHOD_NAME) << std::endl;

	double res = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for (unsigned int fidx=0;fi != fe; ++fi,++fidx)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			double ww = fi->weight * fi->itr->frequency();
			res += ww;
			out << string_format( _TXT( "[%u] result=%f, ff=%u"),
						fidx, ww, (unsigned int)fi->itr->frequency()) << std::endl;			
		}
	}
	out << string_format( _TXT( "sum result=%f"), res) << std::endl;
	return out.str();
}

static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceTermFrequency::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		addNumericParameter( name, parameterValue( name, value));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to weighting function '%s': %s"), METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceTermFrequency::addNumericParameter( const std::string& name, const NumericVariant&)
{
	if (strus::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

WeightingFunctionContextInterface* WeightingFunctionInstanceTermFrequency::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new WeightingFunctionContextTermFrequency( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceTermFrequency::tostring() const
{
	return std::string();
}


WeightingFunctionInstanceInterface* WeightingFunctionTermFrequency::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceTermFrequency( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionTermFrequency::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Calculate the weight of a document as sum of the feature frequency of a feature multiplied with the feature weight"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Numeric, "weight", _TXT( "defines the query feature weight factor"), "0:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

