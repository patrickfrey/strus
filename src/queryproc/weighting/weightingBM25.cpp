/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingBM25.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/constants.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;
#define THIS_METHOD_NAME "BM25"
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextBM25::WeightingFunctionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const WeightingFunctionParameterBM25& parameter_,
		double nofCollectionDocuments_,
		const std::string& attribute_doclen_,
		ErrorBufferInterface* errorhnd_)
	:m_parameter(parameter_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_featar(),m_metadata(metadata_)
	,m_metadata_doclen(-1)
	,m_errorhnd(errorhnd_)
{
	if (m_parameter.b)
	{
		m_metadata_doclen = metadata_->elementHandle( attribute_doclen_.empty()?std::string("doclen"):attribute_doclen_);
		if (m_metadata_doclen<0)
		{
			throw strus::runtime_error( "%s",  _TXT("parameter 'b' set, but no meta data element for the document length defined"));
		}
	}
}

void WeightingFunctionContextBM25::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void WeightingFunctionContextBM25::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		double weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			double nofMatches = stats_.documentFrequency()>=0?stats_.documentFrequency():itr_->documentFrequency();
			double idf = 0.0;
		
			if (m_nofCollectionDocuments > nofMatches * 2)
			{
				idf = std::log10(
						(m_nofCollectionDocuments - nofMatches + 0.5)
						/ (nofMatches + 0.5));
			}
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_featar.push_back( Feature( itr_, weight_, idf));
		}
		else
		{
			throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}


double WeightingFunctionContextBM25::featureWeight( const Feature& feat, const Index& docno) const
{
	if (docno==feat.itr->skipDoc( docno))
	{
		double ff = feat.itr->frequency();
		if (ff == 0.0)
		{
		}
		else if (m_parameter.b)
		{
			m_metadata->skipDoc( docno);
			double doclen = m_metadata->getValue( m_metadata_doclen);
			double rel_doclen = (doclen+1) / m_parameter.avgDocLength;
			return feat.weight * feat.idf
				* (ff * (m_parameter.k1 + 1.0))
				/ (ff + m_parameter.k1 
					* (1.0 - m_parameter.b + m_parameter.b * rel_doclen));
		}
		else
		{
			return feat.weight * feat.idf
				* (ff * (m_parameter.k1 + 1.0))
				/ (ff + m_parameter.k1 * 1.0);
		}
	}
	return 0.0;
}

double WeightingFunctionContextBM25::call( const Index& docno)
{
	double rt = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for ( ;fi != fe; ++fi)
	{
		rt += featureWeight( *fi, docno);
	}
	return rt;
}

std::string WeightingFunctionContextBM25::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);

	out << string_format( _TXT( "calculate %s"), THIS_METHOD_NAME) << std::endl;
	double res = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for (int fidx=0 ;fi != fe; ++fi,++fidx)
	{
		double ww = featureWeight( *fi, docno);
		if (ww < std::numeric_limits<double>::epsilon()) continue;

		unsigned int doclen = 0;
		if (m_parameter.b)
		{
			m_metadata->skipDoc( docno);
			doclen = m_metadata->getValue( m_metadata_doclen).touint();
		}
		out << string_format( _TXT("[%u] result=%f, ff=%u, idf=%f, weight=%f, doclen=%u"),
					fidx, ww, (unsigned int)fi->itr->frequency(), fi->idf,
					fi->weight, doclen) << std::endl;
		res += ww;
	}
	out << string_format( _TXT("sum result=%f"), res) << std::endl;
	return out.str();
}

static NumericVariant parameterValue( const std::string& name, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name, "metadata_doclen"))
		{
			m_metadata_doclen = value;
			if (value.empty()) m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("empty value passed as '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name.c_str());
		}
		else if (strus::caseInsensitiveEquals( name, "k1")
		||  strus::caseInsensitiveEquals( name, "b")
		||  strus::caseInsensitiveEquals( name, "avgdoclen"))
		{
			addNumericParameter( name, parameterValue( name, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name, "k1"))
	{
		m_parameter.k1 = (double)value;
	}
	else if (strus::caseInsensitiveEquals( name, "b"))
	{
		m_parameter.b = (double)value;
	}
	else if (strus::caseInsensitiveEquals( name, "avgdoclen"))
	{
		m_parameter.avgDocLength = (double)value;
	}
	else
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name.c_str());
	}
}


WeightingFunctionContextInterface* WeightingFunctionInstanceBM25::createFunctionContext(
		const StorageClientInterface* storage_,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextBM25( storage_, metadata, m_parameter, nofdocs, m_metadata_doclen, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceBM25::tostring() const
{
	
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_parameter.b << ", k1=" << m_parameter.k1 << ", avgdoclen=" << m_parameter.avgDocLength
			<< ", metadata_doclen=" << m_metadata_doclen
		;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' weighting function to string: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionBM25::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new WeightingFunctionInstanceBM25( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

FunctionDescription WeightingFunctionBM25::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt(_TXT("Calculate the document weight with the weighting scheme \"BM25\""));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Numeric, "k1", _TXT("parameter of the BM25 weighting scheme"), "1:1000");
		rt( P::Numeric, "b", _TXT("parameter of the BM25 weighting scheme"), "0.0001:1000");
		rt( P::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( P::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

