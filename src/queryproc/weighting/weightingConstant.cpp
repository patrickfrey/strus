/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingConstant.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/numericVariant.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <iostream>
#include <iomanip>

using namespace strus;

void WeightingFunctionContextConstant::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (utils::caseInsensitiveEquals( name_, "match"))
		{
			m_featar.push_back( Feature( itr_, weight_));
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function feature parameter '%s'"), "constant", name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' weighting function: %s"), "constant", *m_errorhnd);
}

double WeightingFunctionContextConstant::call( const Index& docno)
{
	double rt = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for (;fi != fe; ++fi)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			rt += fi->weight * m_weight;
		}
	}
	return rt;
}


static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceConstant::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		addNumericParameter( name, parameterValue( name, value));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' weighting function: %s"), "constant", *m_errorhnd);
}

void WeightingFunctionInstanceConstant::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), "constant");
	}
	else if (utils::caseInsensitiveEquals( name, "weight"))
	{
		m_weight = (double)value;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), "Constant", name.c_str());
	}
}

WeightingFunctionContextInterface* WeightingFunctionInstanceConstant::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new WeightingFunctionContextConstant( m_weight, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), "constant", *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceConstant::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "weight=" << m_weight;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping weighting function '%s' to string: %s"), "constant", *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionConstant::createInstance() const
{
	try
	{
		return new WeightingFunctionInstanceConstant( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), "constant", *m_errorhnd, 0);
}

WeightingFunctionInterface::Description WeightingFunctionConstant::getDescription() const
{
	try
	{
		Description rt(_TXT("Calculate the weight of a document as sum of the the feature weights multiplied with their feature frequency"));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features to weight"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), "BM25", *m_errorhnd, WeightingFunctionInterface::Description());
}

