/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingMetadata.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("metadata")

WeightingFunctionContextMetadata::WeightingFunctionContextMetadata(
		MetaDataReaderInterface* metadata_,
		const std::string& elementName_,
		double weight_,
		ErrorBufferInterface* errorhnd_)
	:m_metadata(metadata_)
	,m_elementHandle(-1)
	,m_weight(weight_)
	,m_errorhnd(errorhnd_)
{
	m_elementHandle = m_metadata->elementHandle(elementName_);
	if (m_elementHandle < 0)
	{
		throw strus::runtime_error( _TXT("metadata element '%s' is not defined"), elementName_.c_str());
	}
}

void WeightingFunctionContextMetadata::setVariableValue( const std::string& name_, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void WeightingFunctionContextMetadata::addWeightingFeature(
		const std::string&,
		PostingIteratorInterface*,
		double,
		const TermStatistics&)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("passing feature parameter to weighting function '%s' that has no feature parameters"), THIS_METHOD_NAME);
}

double WeightingFunctionContextMetadata::call( const Index& docno)
{
	m_metadata->skipDoc( docno);
	return m_weight * (double)m_metadata->getValue( m_elementHandle);
}

std::string WeightingFunctionContextMetadata::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "calculate %s"), THIS_METHOD_NAME) << std::endl;

	m_metadata->skipDoc( docno);
	double val = (double)m_metadata->getValue( m_elementHandle);
	double res = m_weight * val;

	out << string_format( _TXT( "result=%f, value=%f"), res, val) << std::endl;
	return out.str();
}


static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceMetadata::setMaxNofWeightedFields( int N)
{
	if (N != 1) m_errorhnd->report( ErrorCodeNotImplemented, _TXT("set maximum number of weighting fields not implemented for the function '%s'"), THIS_METHOD_NAME);
}

void WeightingFunctionInstanceMetadata::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "name"))
		{
			m_elementName = value;
		}
		else
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceMetadata::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "weight"))
		{
			m_weight = (double)value;
		}
		else if (strus::caseInsensitiveEquals( name_, "name"))
		{
			throw strus::runtime_error( _TXT("illegal numeric type for '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding numeric parameter to weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd);
}

WeightingFunctionContextInterface* WeightingFunctionInstanceMetadata::createFunctionContext(
		const StorageClientInterface* storage,
		const GlobalStatistics&) const
{
	try
	{
		if (m_elementName.empty())
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("undefined '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, "name");
		}
		strus::Reference<MetaDataReaderInterface> metadata( storage->createMetaDataReader());
		if (!metadata.get()) throw strus::runtime_error(_TXT("failed to create meta data reader: %s"), m_errorhnd->fetchError());

		return new WeightingFunctionContextMetadata( metadata.release(), m_elementName, m_weight, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceMetadata::view() const
{
	try
	{
		StructView rt;
		rt( "name", m_elementName);
		rt( "weight", m_weight);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

WeightingFunctionInstanceInterface* WeightingFunctionMetadata::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceMetadata( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


StructView WeightingFunctionMetadata::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the weight of a document as value of a meta data element."));
		rt( P::Metadata, "name", _TXT( "name of the meta data element to use as weight"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

