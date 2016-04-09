/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingMetadata.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/numericVariant.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <iostream>
#include <iomanip>

using namespace strus;

WeightingFunctionContextMetadata::WeightingFunctionContextMetadata(
		MetaDataReaderInterface* metadata_,
		const std::string& elementName_,
		double weight_,
		ErrorBufferInterface* errorhnd_)
	:m_metadata(metadata_)
	,m_elementHandle(metadata_->elementHandle(elementName_))
	,m_weight(weight_)
	,m_errorhnd(errorhnd_)
{}

void WeightingFunctionContextMetadata::addWeightingFeature(
		const std::string&,
		PostingIteratorInterface*,
		double,
		const TermStatistics&)
{
	m_errorhnd->report( _TXT("passing feature parameter to weighting function '%s' that has no feature parameters"), "metadata");
}

double WeightingFunctionContextMetadata::call( const Index& docno)
{
	m_metadata->skipDoc( docno);
	return m_weight * (double)m_metadata->getValue( m_elementHandle);
}

static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceMetadata::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			m_elementName = value;
		}
		else
		{
			addNumericParameter( name, parameterValue( name, value));
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to weighting function '%s': %s"), "metadata", *m_errorhnd);
}

void WeightingFunctionInstanceMetadata::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "weight"))
		{
			m_weight = (double)value;
		}
		else if (utils::caseInsensitiveEquals( name, "name"))
		{
			throw strus::runtime_error( _TXT("illegal numeric type for '%s' weighting function parameter '%s'"), "metadata", name.c_str());
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), "metadata", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding numeric parameter to weighting function '%s': %s"), "metadata", *m_errorhnd);
}

WeightingFunctionContextInterface* WeightingFunctionInstanceMetadata::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface* metadata_,
		const GlobalStatistics&) const
{
	try
	{
		if (m_elementName.empty())
		{
			m_errorhnd->report( _TXT("undefined '%s' weighting function parameter '%s'"), "metadata", "name");
		}
		return new WeightingFunctionContextMetadata( metadata_, m_elementName, m_weight, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), "metadata", *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceMetadata::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "name=" << m_elementName << ", weight=" << m_weight;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping weighting function '%s' to string: %s"), "metadata", *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionMetadata::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceMetadata( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), "metadata", *m_errorhnd, 0);
}


FunctionDescription WeightingFunctionMetadata::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Calculate the weight of a document as value of a meta data element."));
		rt( P::Metadata, "name", _TXT( "name of the meta data element to use as weight"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), "metadata", *m_errorhnd, FunctionDescription());
}

