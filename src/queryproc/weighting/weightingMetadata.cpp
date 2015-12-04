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
#include "weightingMetadata.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <iostream>
#include <iomanip>

using namespace strus;

WeightingFunctionContextMetadata::WeightingFunctionContextMetadata(
		MetaDataReaderInterface* metadata_,
		const std::string& elementName_,
		float weight_,
		ErrorBufferInterface* errorhnd_)
	:m_metadata(metadata_)
	,m_elementHandle(metadata_->elementHandle(elementName_))
	,m_weight(weight_)
	,m_errorhnd(errorhnd_)
{}

void WeightingFunctionContextMetadata::addWeightingFeature(
		const std::string&,
		PostingIteratorInterface*,
		float)
{
	m_errorhnd->report( _TXT("passing feature parameter to weighting function '%s' that has no feature parameters"), "metadata");
}

float WeightingFunctionContextMetadata::call( const Index& docno)
{
	m_metadata->skipDoc( docno);
	return m_weight * (double)m_metadata->getValue( m_elementHandle);
}

static ArithmeticVariant parameterValue( const std::string& name, const std::string& value)
{
	ArithmeticVariant rt;
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

void WeightingFunctionInstanceMetadata::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
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


WeightingFunctionInstanceInterface* WeightingFunctionMetadata::createInstance() const
{
	try
	{
		return new WeightingFunctionInstanceMetadata( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), "metadata", *m_errorhnd, 0);
}


