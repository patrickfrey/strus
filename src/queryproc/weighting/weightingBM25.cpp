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
#include <cmath>
#include <ctime>
#include <iostream>

using namespace strus;
#define WEIGHTING_SCHEME_NAME "BM25"
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextBM25::WeightingFunctionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		double k1_,
		double b_,
		double avgDocLength_,
		double nofCollectionDocuments_,
		const std::string& attribute_doclen_,
		ErrorBufferInterface* errorhnd_)
	:m_k1(k1_),m_b(b_),m_avgDocLength(avgDocLength_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_featar(),m_metadata(metadata_)
	,m_metadata_doclen(metadata_->elementHandle( attribute_doclen_.empty()?std::string("doclen"):attribute_doclen_))
	,m_errorhnd(errorhnd_)
{
	if (m_metadata_doclen<0)
	{
		throw strus::runtime_error( _TXT("no meta data element for the document lenght defined"));
	}
}

void WeightingFunctionContextBM25::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_,
		const TermStatistics& stats_)
{
	try
	{
		if (utils::caseInsensitiveEquals( name_, "match"))
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
			throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), WEIGHTING_SCHEME_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd);
}


double WeightingFunctionContextBM25::call( const Index& docno)
{
	double rt = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for ( ;fi != fe; ++fi)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			double ff = fi->itr->frequency();
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ff=" << ff << std::endl;
#endif
			if (ff == 0.0)
			{
			}
			else if (m_b)
			{
				m_metadata->skipDoc( docno);
				double doclen = m_metadata->getValue( m_metadata_doclen);
				double rel_doclen = (doclen+1) / m_avgDocLength;
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = fi->weight * fi->idf
						* (ff * (m_k1 + 1.0))
						/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
				std::cout << "idf[" << (int)(fi-m_featar.begin()) << "]=" << fi->idf << " doclen=" << doclen << " reldoclen=" << rel_doclen << " weight=" << ww << std::endl;
#endif
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
			}
			else
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				double ww = fi->weight * fi->idf
						* (ff * (m_k1 + 1.0))
						/ (ff + m_k1 * 1.0);
				std::cout << "idf[" << (int)(fi-m_featar.begin()) << "]=" << fi->idf << " weight=" << ww << std::endl;
#endif
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * 1.0);
			}
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "document weight=" << rt << std::endl;
#endif
	return rt;
}

static ArithmeticVariant parameterValue( const std::string& name, const std::string& value)
{
	ArithmeticVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
		}
		else if (utils::caseInsensitiveEquals( name, "metadata_doclen"))
		{
			m_metadata_doclen = value;
			if (value.empty()) m_errorhnd->report( _TXT("empty value passed as '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
		else if (utils::caseInsensitiveEquals( name, "k1")
		||  utils::caseInsensitiveEquals( name, "b")
		||  utils::caseInsensitiveEquals( name, "avgdoclen"))
		{
			addNumericParameter( name, parameterValue( name, value));
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name.c_str(), WEIGHTING_SCHEME_NAME);
	}
	else if (utils::caseInsensitiveEquals( name, "k1"))
	{
		m_k1 = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "b"))
	{
		m_b = (double)value;
	}
	else if (utils::caseInsensitiveEquals( name, "avgdoclen"))
	{
		m_avgdoclen = (double)value;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' weighting function parameter '%s'"), WEIGHTING_SCHEME_NAME, name.c_str());
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
		return new WeightingFunctionContextBM25( storage_, metadata, m_k1, m_b, m_avgdoclen, nofdocs, m_metadata_doclen, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

std::string WeightingFunctionInstanceBM25::tostring() const
{
	
	try
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_b << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen
			<< ", metadata_doclen=" << m_metadata_doclen
		;
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' weighting function to string: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, std::string());
}


WeightingFunctionInstanceInterface* WeightingFunctionBM25::createInstance() const
{
	try
	{
		return new WeightingFunctionInstanceBM25( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, 0);
}

WeightingFunctionInterface::Description WeightingFunctionBM25::getDescription() const
{
	try
	{
		Description rt(_TXT("Calculate the document weight with the weighting scheme \"BM25\""));
		rt( Description::Param::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( Description::Param::Numeric, "k1", _TXT("parameter of the BM25 weighting scheme"), "1:1000");
		rt( Description::Param::Numeric, "b", _TXT("parameter of the BM25 weighting scheme"), "0.0001:1000");
		rt( Description::Param::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( Description::Param::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), WEIGHTING_SCHEME_NAME, *m_errorhnd, WeightingFunctionInterface::Description());
}

