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
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define METHOD_NAME "constant"

void WeightingFunctionContextConstant::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		double weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (utils::caseInsensitiveEquals( name_, "match"))
		{
			if (m_precalc)
			{
				for (Index docno=itr_->skipDoc(0); docno; docno=itr_->skipDoc(docno+1))
				{
					m_precalcmap[ docno] += weight_ * m_weight;
				}
			}
			m_featar.push_back( Feature( itr_, weight_));
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' weighting function feature parameter '%s'"), METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' weighting function: %s"), METHOD_NAME, *m_errorhnd);
}

double WeightingFunctionContextConstant::call( const Index& docno)
{
	double rt = 0.0;
	if (m_precalc)
	{
		std::map<Index,double>::const_iterator mi = m_precalcmap.find( docno);
		if (mi != m_precalcmap.end())
		{
			return mi->second;
		}
		else
		{
			return 0.0;
		}
	}
	else
	{
		std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
		for (;fi != fe; ++fi)
		{
			if (docno==fi->itr->skipDoc( docno))
			{
				rt += fi->weight * m_weight;
			}
		}
	}
	return rt;
}

std::string WeightingFunctionContextConstant::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);

	out << string_format( _TXT( "calculate %s"), METHOD_NAME) << std::endl;
	double res_precalc = 0.0;
	if (m_precalc)
	{
		std::map<Index,double>::const_iterator mi = m_precalcmap.find( docno);
		if (mi != m_precalcmap.end())
		{
			res_precalc = mi->second;
		}
	}
	double res_detail = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for (unsigned int fidx=0;fi != fe; ++fi,++fidx)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			double ww = fi->weight * m_weight;
			res_detail += ww;
			out << string_format( _TXT( "[%u] result=%f"), fidx, ww) << std::endl;
		}
	}
	if (m_precalc)
	{
		out << string_format( _TXT( "sum nof features=%u, result=%f, precalc=%f"), (unsigned int)m_featar.size(), res_detail, res_precalc) << std::endl;
	}
	else
	{
		out << string_format( _TXT( "sum nof features=%u, result=%f"), (unsigned int)m_featar.size(), res_detail) << std::endl;
	}
	return out.str();
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
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' weighting function: %s"), METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceConstant::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "precalc"))
	{
		m_precalc = value.toint();
	}
	else if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), METHOD_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "weight"))
	{
		m_weight = (double)value;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

WeightingFunctionContextInterface* WeightingFunctionInstanceConstant::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new WeightingFunctionContextConstant( m_weight, m_precalc, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceConstant::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "weight=" << m_weight << ", " << "precalc=" << (m_precalc?"1":"0");
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping weighting function '%s' to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionConstant::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceConstant( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of weighting function '%s': %s"), METHOD_NAME, *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionConstant::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt(_TXT("Calculate the weight of a document as sum of the the feature weights multiplied with their feature frequency"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), "BM25", *m_errorhnd, FunctionDescription());
}

