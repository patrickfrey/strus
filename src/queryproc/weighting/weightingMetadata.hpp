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
#ifndef _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#define _STRUS_WEIGHTING_METADATA_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingExecutionContextInterface.hpp"
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunctionMetadata;


/// \class WeightingExecutionContextMetadata
/// \brief Weighting function ExecutionContext for the metadata weighting function
class WeightingExecutionContextMetadata
	:public WeightingExecutionContextInterface
{
public:
	WeightingExecutionContextMetadata(
			MetaDataReaderInterface* metadata_,
			const std::string& elementName_,
			float weight_)
		:m_metadata(metadata_)
		,m_elementHandle(metadata_->elementHandle(elementName_))
		,m_weight(weight_)
	{}

	virtual void addWeightingFeature(
			const std::string&,
			PostingIteratorInterface*,
			float)
	{
		throw strus::runtime_error( _TXT("passing feature parameter to weighting function '%s' that has no feature parameters"), "metadata");
	}

	virtual float call( const Index& docno)
	{
		m_metadata->skipDoc( docno);
		return m_weight * (float)m_metadata->getValue( m_elementHandle);
	}

private:
	MetaDataReaderInterface* m_metadata;
	Index m_elementHandle;
	float m_weight;
};

/// \class WeightingFunctionInstanceMetadata
/// \brief Weighting function instance for a weighting that returns a metadata element for every matching document
class WeightingFunctionInstanceMetadata
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceMetadata()
		:m_weight(1.0){}

	virtual ~WeightingFunctionInstanceMetadata(){}

	virtual void addStringParameter( const std::string& name, const std::string& value)
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			m_elementName = value;
		}
		else
		{
			addNumericParameter( name, arithmeticVariantFromString( value));
		}
	}

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)
	{
		if (utils::caseInsensitiveEquals( name, "weight"))
		{
			m_weight = (float)value;
		}
		else if (utils::caseInsensitiveEquals( name, "name"))
		{
			throw strus::runtime_error( _TXT("illegal numeric type for '%s' weighting function parameter '%s'"), "metadata", name.c_str());
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "metadata", name.c_str());
		}
	}

	virtual bool isFeatureParameter( const std::string&) const
	{
		return false;
	}

	virtual WeightingExecutionContextInterface* createExecutionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface* metadata_) const
	{
		if (m_elementName.empty())
		{
			throw strus::runtime_error( _TXT("undefined '%s' weighting function parameter '%s'"), "metadata", "name");
		}
		return new WeightingExecutionContextMetadata( metadata_, m_elementName, m_weight);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "name=" << m_elementName << ", weight=" << m_weight;
		return rt.str();
	}

private:
	float m_weight;
	std::string m_elementName;
};


/// \class WeightingFunctionMetadata
/// \brief Weighting function that simply returns the value of a meta data element multiplied by a weight
class WeightingFunctionMetadata
	:public WeightingFunctionInterface
{
public:
	WeightingFunctionMetadata(){}
	virtual ~WeightingFunctionMetadata(){}


	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstanceMetadata();
	}
};

}//namespace
#endif

