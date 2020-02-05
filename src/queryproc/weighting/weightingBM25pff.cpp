/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingBM25pff.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "strus/constants.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "viewUtils.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("BM25pff")

WeightingFunctionContextBM25pff::WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		const WeightingFunctionParameterBM25pff& parameter_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		const std::string& structname_,
		ErrorBufferInterface* errorhnd_)
	:m_proximityWeightingContext(parameter_.proximityConfig)
	,m_parameter(parameter_)
	,m_itrarsize(0)
	,m_weightar()
	,m_stopword_itrarsize(0)
	,m_stopword_weightar()
	,m_eos_itr(0)
	,m_structitr(storage->createStructIterator())
	,m_metadata(storage->createMetaDataReader())
	,m_metadata_doclen(-1)
	,m_structno(0)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_storage(storage)
	,m_errorhnd(errorhnd_)
{
	if (!m_structitr.get())
	{
		throw strus::runtime_error( _TXT("failed to create structure iterator: %s"), m_errorhnd->fetchError());
	}
	if (!m_metadata.get())
	{
		throw strus::runtime_error( _TXT("failed to create meta data reader: %s"), m_errorhnd->fetchError());
	}
	m_metadata_doclen = m_metadata->elementHandle( metadata_doclen_);
	if (m_metadata_doclen<0)
	{
		throw strus::runtime_error( _TXT("no meta data element '%s' for the document lenght defined"), metadata_doclen_.c_str());
	}
	if (!structname_.empty())
	{
		m_structno = m_storage->structTypeNumber( structname_);
		if (!m_structno)
		{
			m_errorhnd->info( _TXT("no structure type name '%s' defined in storage, using all structures"), structname_.c_str());
		}
	}
	std::memset( &m_itrar, 0, sizeof(m_itrar));
	std::memset( &m_stopword_itrar, 0, sizeof(m_itrar));
}

void WeightingFunctionContextBM25pff::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void WeightingFunctionContextBM25pff::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "punct"))
		{
			if (m_eos_itr) throw std::runtime_error( _TXT("only one end of sentence marker feature allowed"));
			m_eos_itr = itr;
		}
		else if (strus::caseInsensitiveEquals( name_, "match"))
		{
			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = strus::Math::log10( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));
			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			if (m_parameter.maxdf * m_nofCollectionDocuments < df)
			{
				if (m_stopword_itrarsize >= MaxNofArguments) throw std::runtime_error( _TXT("number of weighting features out of range"));
				m_stopword_itrar[ m_stopword_itrarsize] = itr;
				m_stopword_weightar[ m_stopword_itrarsize] = idf * weight;
				++m_stopword_itrarsize;
			}
			else
			{
				if (m_itrarsize >= MaxNofArguments) throw std::runtime_error( _TXT("number of weighting features out of range"));
				m_itrar[ m_itrarsize] = itr;
				m_weightar[ m_itrarsize] = idf * weight;
				++m_itrarsize;
			}
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

double WeightingFunctionParameterBM25pff::postingsWeight( int doclen, double featureWeight, double ff) const
{
	if (b)
	{
		double rel_doclen = (double)doclen / avgDocLength;
		double ww = featureWeight
				* (ff * (k1 + 1.0))
				/ (ff + k1
					* (1.0 - b + b * rel_doclen));
		return ww;
	}
	else
	{
		double ww = featureWeight
				* (ff * (k1 + 1.0))
				/ (ff + k1 * 1.0);
		return ww;
	}
	return 0.0;
}

const std::vector<WeightedField>& WeightingFunctionContextBM25pff::call( const Index& docno)
{
	try
	{
		m_metadata->skipDoc( docno);
		int doclen = m_metadata->getValue( m_metadata_doclen).toint();
		m_lastResult.resize( 0);

		if (m_itrarsize > 1)
		{
			//... more than one relevant feature, do weight with proximity weighted ff and structures
			m_proximityWeightingContext.init( m_itrar, m_itrarsize, m_eos_itr, docno, strus::IndexRange()/*no field, weight all*/);
			m_proximityWeightingContext.initStructures( m_structitr.get(), m_structno);
			m_proximityWeightingContext.collectFieldStatistics();

			std::vector<FieldStatistics>::const_iterator
				si = m_proximityWeightingContext.stats_begin(),
				se = m_proximityWeightingContext.stats_end();
			m_lastResult.reserve( se-si);
			for (; si != se; ++si)
			{
				// Stopword weight part (estimate):
				int subdoclen = si->field.defined() ? (si->field.end() - si->field.start()) : doclen;
				double ww = 0.0;
				int pi,pe;
				for (pi=0,pe=m_stopword_itrarsize; pi<pe; ++pi)
				{
					ww += m_parameter.postingsWeight( subdoclen, m_stopword_weightar[ pi], m_stopword_itrar[ pi]->frequency());
				}
				// Proximity weight part (calculated):
				for (pi=0,pe=m_itrarsize; pi<pe; ++pi)
				{
					ww += m_parameter.postingsWeight( subdoclen, m_weightar[ pi], si->ff[ m_itrarsize]);
				}
				m_lastResult.push_back( WeightedField( si->field, ww));
			}
		}
		else
		{
			//... only one relevant feature, fallback to BM25
			double ww = 0.0;
			int pi,pe;
			for (pi=0,pe=m_itrarsize; pi<pe; ++pi)
			{
				ww += m_parameter.postingsWeight( doclen, m_weightar[ pi], m_itrar[ pi]->frequency());
			}
			for (pi=0,pe=m_stopword_itrarsize; pi<pe; ++pi)
			{
				ww += m_parameter.postingsWeight( doclen, m_stopword_weightar[ pi], m_stopword_itrar[ pi]->frequency());
			}
			m_lastResult.resize( 1);
			m_lastResult[ 0].setWeight( ww);
		}
		return m_lastResult;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, m_lastResult);
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceBM25pff::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match")
		||  strus::caseInsensitiveEquals( name_, "punct"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "metadata_doclen"))
		{
			if (value.empty()) m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("empty value passed as '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
			m_metadata_doclen = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "struct"))
		{
			if (value.empty()) m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("empty value passed as '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
			m_structname = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "nofres")
			|| strus::caseInsensitiveEquals( name_, "k1")
			|| strus::caseInsensitiveEquals( name_, "b")
			|| strus::caseInsensitiveEquals( name_, "avgdoclen")
			|| strus::caseInsensitiveEquals( name_, "maxdf")
			|| strus::caseInsensitiveEquals( name_, "dist_imm")
			|| strus::caseInsensitiveEquals( name_, "dist_close")
			|| strus::caseInsensitiveEquals( name_, "dist_near")
			|| strus::caseInsensitiveEquals( name_, "cluster")
			|| strus::caseInsensitiveEquals( name_, "hotspots")
			|| strus::caseInsensitiveEquals( name_, "ffbase"))
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceBM25pff::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match")
	||  strus::caseInsensitiveEquals( name_, "punct"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature and not as string or numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "metadata_doclen")
		|| strus::caseInsensitiveEquals( name_, "struct"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "nofres"))
	{
		m_parameter.maxNofResults = value.toint();
	}
	else if (strus::caseInsensitiveEquals( name_, "k1"))
	{
		m_parameter.k1 = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "b"))
	{
		m_parameter.b = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "avgdoclen"))
	{
		m_parameter.avgDocLength = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "maxdf"))
	{
		m_parameter.maxdf = value.tofloat();
	}
	else if (strus::caseInsensitiveEquals( name_, "dist_imm"))
	{
		m_parameter.proximityConfig.setDistanceImm( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "dist_close"))
	{
		m_parameter.proximityConfig.setDistanceClose( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "dist_near"))
	{
		m_parameter.proximityConfig.setDistanceNear( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "cluster"))
	{
		m_parameter.proximityConfig.setMinClusterSize( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "hotspots"))
	{
		m_parameter.proximityConfig.setNofHotspots( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "ffbase"))
	{
		m_parameter.proximityConfig.setMinFfWeight( value.tofloat());
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}


WeightingFunctionContextInterface* WeightingFunctionInstanceBM25pff::createFunctionContext(
		const StorageClientInterface* storage_,
		const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage_->nofDocumentsInserted();
		return new WeightingFunctionContextBM25pff(
				storage_, m_parameter, nofdocs,
				m_metadata_doclen, m_structname, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceBM25pff::view() const
{
	try
	{
		StructView rt;
		rt( "b", m_parameter.b);
		rt( "k1", m_parameter.k1);
		rt( "avgdoclen", m_parameter.avgDocLength);
		rt( "metadata_doclen", m_metadata_doclen);
		rt( "struct", m_structname);
		rt( "maxdf", m_parameter.maxdf);
		rt( "nofres", m_parameter.maxNofResults);
		rt( "dist_imm", m_parameter.proximityConfig.distance_imm);
		rt( "dist_close", m_parameter.proximityConfig.distance_close);
		rt( "dist_near", m_parameter.proximityConfig.distance_near);
		rt( "cluster", m_parameter.proximityConfig.minClusterSize);
		rt( "hotspots", m_parameter.proximityConfig.nofHotspots);
		rt( "ffbase", m_parameter.proximityConfig.minFfWeight);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' weighting function introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

const char* WeightingFunctionInstanceBM25pff::name() const
{
	return THIS_METHOD_NAME;
}

WeightingFunctionInstanceInterface* WeightingFunctionBM25pff::createInstance(
		const QueryProcessorInterface* ) const
{
	try
	{
		return new WeightingFunctionInstanceBM25pff( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' function instance: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionBM25pff::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the document weight with the weighting scheme \"BM25pff\". This is \"BM25\" where the feature frequency is counted by 1.0 per feature only for features with the maximum proximity score. The proximity score is a measure that takes the proximity of other query features into account"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Feature, "punct", _TXT( "defines the delimiter for sentences"), "");
		rt( P::String, "struct", _TXT( "name of the structure type to use for determining contents to weight (use all if undefined)"), "");
		rt( P::Numeric, "k1", _TXT("parameter of the BM25pff weighting scheme"), "1:1000");
		rt( P::Numeric, "b", _TXT("parameter of the BM25pff weighting scheme"), "0.0001:1000");
		rt( P::Numeric, "avgdoclen", _TXT("the average document lenght"), "0:");
		rt( P::Numeric, "maxdf", _TXT("the maximum df for a feature to be not considered as stopword as fraction of the collection size"), "0:");
		rt( P::Numeric, "nofres", _TXT( "maximum number of weighted fields returned by a weighting function call for one document"), "1:");
		rt( P::Numeric, "dist_imm", _TXT( "ordinal position distance considered as immediate in the same sentence"), "1:");
		rt( P::Numeric, "dist_close", _TXT( "ordinal position distance considered as close in the same sentence"), "1:");
		rt( P::Numeric, "dist_near", _TXT( "ordinal position distance considered as near for features not in the same sentence"), "1:");
		rt( P::Numeric, "cluster", _TXT( "part [0.0,1.0] of query features considered as relevant in a group"), "1:");
		rt( P::Numeric, "hotspots", _TXT( "number that defines the number of features with most neighbour features to be used to determine the document parts to weight"), "1:");
		rt( P::Numeric, "ffbase", _TXT( "value in the range from 0.0 to 1.0 specifying the minimum feature occurrence value assigned to a feature"), "0.0:1.0");
		rt( P::Metadata, "metadata_doclen", _TXT("the meta data element name referencing the document lenght for each document weighted if not defined by structures"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* WeightingFunctionBM25pff::name() const
{
	return THIS_METHOD_NAME;
}


