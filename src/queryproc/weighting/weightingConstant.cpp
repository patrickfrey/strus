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
#include "weightingConstant.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/arithmeticVariant.hpp"
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

float WeightingFunctionContextConstant::call( const Index& docno)
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


static ArithmeticVariant parameterValue( const std::string& name, const std::string& value)
{
	ArithmeticVariant rt;
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

void WeightingFunctionInstanceConstant::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
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


