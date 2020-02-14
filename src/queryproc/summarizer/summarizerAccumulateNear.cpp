/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerAccumulateNear.hpp"
#include "postingIteratorLink.hpp"
#include "weightedValue.hpp"
#include "private/ranker.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "strus/base/configParser.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include "textNormalizer.hpp"
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("matchnear")

SummarizerFunctionContextAccumulateNear::SummarizerFunctionContextAccumulateNear(
		const StorageClientInterface* storage_,
		const SummarizerFunctionParameterAccumulateNear& parameter_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_proximityWeightingContext(parameter_.proximityConfig)
	,m_parameter(parameter_)
	,m_itrarsize(0)
	,m_weightar()
	,m_stopword_itrarsize(0)
	,m_stopword_weightar()
	,m_eos_itr(0)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_storage(storage_)
	,m_collectors()
	,m_weightnorm(1.0)
	,m_errorhnd(errorhnd_)
{
	typedef SummarizerFunctionParameterAccumulateNear::CollectorConfig CollectorConfig;
	std::vector<CollectorConfig>::const_iterator
		ci = m_parameter.collectorConfigs.begin(), ce = m_parameter.collectorConfigs.end();
	for (; ci != ce; ++ci)
	{
		m_collectors.push_back( ForwardIndexCollector( m_storage, ci->tagSeparator, ci->tagType, m_errorhnd));
		std::vector<std::string>::const_iterator ti = ci->collectTypes.begin(), te = ci->collectTypes.end();
		for (; ti != te; ++ti)
		{
			m_collectors.back().addFeatureType( *ti);
		}
	}
	std::memset( &m_itrar, 0, sizeof(m_itrar));
	std::memset( &m_stopword_itrar, 0, sizeof(m_itrar));
}

void SummarizerFunctionContextAccumulateNear::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextAccumulateNear::addSummarizationFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
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
				double ww = idf * weight;
				m_stopword_weightar[ m_stopword_itrarsize] = ww;
				++m_stopword_itrarsize;
			}
			else
			{
				if (m_itrarsize >= MaxNofArguments) throw std::runtime_error( _TXT("number of weighting features out of range"));
				m_itrar[ m_itrarsize] = itr;
				m_weightar[ m_itrarsize] = idf * weight;
				++m_itrarsize;

				m_weightnorm = 0.0;
				for (std::size_t ii=0; ii<m_itrarsize; ++ii)
				{
					m_weightnorm += m_weightar[ ii] * m_weightar[ ii];
				}
				m_weightnorm = std::sqrt( m_weightnorm);
			}
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionContextAccumulateNear::collectSummariesFromEntityMap( std::vector<SummaryElement>& summaries, const std::string& name, EntityMap& entitymap) const
{
	typedef WeightedValue<const char*> WeightedFeature;

	Ranker<WeightedFeature> ranker( m_parameter.maxNofResults);
	EntityMap::const_iterator ei = entitymap.begin(), ee = entitymap.end();
	for (; ei != ee; ++ei)
	{
		ranker.insert( WeightedFeature( ei->second / m_weightnorm, ei->first.c_str()));
	}
	std::vector<WeightedFeature> result = ranker.result();
	std::vector<WeightedFeature>::const_iterator ri = result.begin(), re = result.end();
	for (; ri != re; ++ri)
	{
		summaries.push_back( SummaryElement( name, ri->value, ri->weight));
	}
}

std::vector<SummaryElement>
	SummarizerFunctionContextAccumulateNear::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		std::vector<WeightedNeighbour> weightedNeighbours;
		if (m_itrarsize == 0)
		{
			return std::vector<SummaryElement>();
		}
		else if (m_itrarsize == 1)
		{
			weightedNeighbours = ProximityWeightingContext::getWeightedNeighboursForSingleFeature( 
					m_parameter.distance_collect,
					m_itrar[0], m_eos_itr, m_weightar[ 0],
					doc.docno(), doc.field());
		}
		else
		{
			m_proximityWeightingContext.init(
					m_itrar, m_itrarsize, m_eos_itr,
					doc.docno(), doc.field());
			weightedNeighbours = m_proximityWeightingContext.getWeightedNeighbours( 
					m_weightar, m_parameter.distance_collect);
		}
		std::vector<ForwardIndexCollector>::iterator
			ci = m_collectors.begin(), ce = m_collectors.end();
		for (int cidx=0; ci != ce; ++ci,++cidx)
		{
			ci->skipDoc( doc.docno());

			EntityMap entitymap;
			std::vector<WeightedNeighbour>::const_iterator
				wi = weightedNeighbours.begin(), we = weightedNeighbours.end();
			while (wi != we)
			{
				strus::Index wpos = ci->skipPos( wi->pos);
				for (; wi != we && wi->pos < wpos; ++wi){}
				if (wi == we) break;
				if (wi->pos == wpos)
				{
					std::string elem = 
						strus::stripForwardIndexText(
							ci->fetch(), 
							m_parameter.collectorConfigs[ cidx].stripCharacters);
					entitymap[ elem] += wi->weight;
				}
				++wi;
			}
			collectSummariesFromEntityMap( rt, m_parameter.collectorConfigs[ cidx].name, entitymap);
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

const char* SummarizerFunctionInstanceAccumulateNear::name() const
{
	return THIS_METHOD_NAME;
}

void SummarizerFunctionParameterAccumulateNear::addConfig( const std::string& configstr_, ErrorBufferInterface* errorhnd)

{
	std::string configstr = configstr_;
	std::string name;
	std::vector<std::string> collectTypes;
	std::string tagType;
	char tagSeparator = '\0';
	if (!extractStringFromConfigString( name, configstr, "name", errorhnd))
	{
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "name", errorhnd->fetchError());
		}
	}
	if (!extractStringArrayFromConfigString( collectTypes, configstr, "type", ',', errorhnd))
	{
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "type", errorhnd->fetchError());
		}
		else
		{
			throw strus::runtime_error(_TXT("missing mandatory '%s' in configuration"), "type");
		}
	}
	if (collectTypes.empty())
	{
		throw strus::runtime_error(_TXT("empty '%s' definition in configuration, nothing to accumulate"), "type");
	}
	if (!extractStringFromConfigString( tagType, configstr, "tag", errorhnd))
	{
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "tag", errorhnd->fetchError());
		}
	}
	std::string stripCharacters;
	if (!extractStringFromConfigString( stripCharacters, configstr, "strip", errorhnd))
	{
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "strip", errorhnd->fetchError());
		}
	}
	std::string tagSeparatorStr;
	if (extractStringFromConfigString( tagSeparatorStr, configstr, "sep", errorhnd))
	{
		if (tagSeparatorStr.empty())
		{
			tagSeparator = '\0';
		}
		else if (tagSeparatorStr.size() == 1)
		{
			tagSeparator = tagSeparatorStr[0];
		}
		else
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "sep", _TXT("single ascii character expected"));
		}
	}
	else
	{
		if (errorhnd->hasError())
		{
			throw strus::runtime_error( _TXT("failed to parse '%s' in configuration: %s"), "sep", errorhnd->fetchError());
		}
	}
	configstr = strus::string_conv::trim( configstr);
	if (!configstr.empty())
	{
		throw strus::runtime_error(_TXT("superfluous definitions int in configuration: '%s'"), configstr.c_str());
	}
	collectorConfigs.push_back( CollectorConfig( name, collectTypes, tagType, stripCharacters, tagSeparator));
}

StructView SummarizerFunctionParameterAccumulateNear::CollectorConfig::view() const
{
	StructView rt;
	rt( "name", name);
	rt( "type", collectTypes);
	rt( "tag", tagType);
	rt( "strip", stripCharacters);
	rt( "sep", tagSeparator);
	return rt;
}

static StructView getStructView( const std::vector<SummarizerFunctionParameterAccumulateNear::CollectorConfig> car)
{
	StructView rt;
	std::vector<SummarizerFunctionParameterAccumulateNear::CollectorConfig>::const_iterator
		ci = car.begin(), ce = car.end();
	for (; ci != ce; ++ci)
	{
		rt( ci->view());
	}
	return rt;
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void SummarizerFunctionInstanceAccumulateNear::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match")
		||  strus::caseInsensitiveEquals( name_, "punct"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer function '%s' expected to be defined as a feature and not as a string or a numeric value"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "collect"))
		{
			m_parameter.addConfig( value, m_errorhnd);
		}
		else if (strus::caseInsensitiveEquals( name_, "results")
			|| strus::caseInsensitiveEquals( name_, "maxdf")
			|| strus::caseInsensitiveEquals( name_, "dist_imm")
			|| strus::caseInsensitiveEquals( name_, "dist_close")
			|| strus::caseInsensitiveEquals( name_, "dist_near")
			|| strus::caseInsensitiveEquals( name_, "dist_collect")
			|| strus::caseInsensitiveEquals( name_, "cluster"))
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarizer function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceAccumulateNear::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match")
	||  strus::caseInsensitiveEquals( name_, "punct"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer function '%s' expected to be defined as a feature and not as a string or a numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "collect"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer function '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "results"))
	{
		m_parameter.maxNofResults = value.toint();
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
	else if (strus::caseInsensitiveEquals( name_, "dist_collect"))
	{
		m_parameter.distance_collect = value.toint();
	}
	else if (strus::caseInsensitiveEquals( name_, "cluster"))
	{
		m_parameter.proximityConfig.setMinClusterSize( value.tofloat());
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarizer function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAccumulateNear::createFunctionContext(
		const StorageClientInterface* storage,
		const GlobalStatistics& stats) const
{
	if (m_parameter.collectorConfigs.empty())
	{
		m_errorhnd->info( _TXT( "warning: no elements defined to collect in '%s'"), THIS_METHOD_NAME);
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextAccumulateNear( storage, m_parameter, nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceAccumulateNear::view() const
{
	try
	{
		StructView rt;
		rt( "collect", getStructView( m_parameter.collectorConfigs));
		rt( "maxdf", m_parameter.maxdf);
		rt( "results", m_parameter.maxNofResults);
		rt( "dist_imm", m_parameter.proximityConfig.distance_imm);
		rt( "dist_close", m_parameter.proximityConfig.distance_close);
		rt( "dist_near", m_parameter.proximityConfig.distance_near);
		rt( "dist_collect", m_parameter.distance_collect);
		rt( "cluster", m_parameter.proximityConfig.minClusterSize);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionInstanceInterface* SummarizerFunctionAccumulateNear::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceAccumulateNear( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

const char* SummarizerFunctionAccumulateNear::name() const
{
	return THIS_METHOD_NAME;
}

StructView SummarizerFunctionAccumulateNear::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Extract and weight neighbour elements in the forward index."));
		rt( P::Feature, "match", _TXT( "defines the query features to use for localization and weighting of the neighbour features to collect"), "");
		rt( P::Feature, "punct", _TXT( "defines the sentence delimiter"), "");
		rt( P::String, "collect", _TXT( "describes a configuration of elements to collect with name,type,tag,sep,strip in the strus configuration string syntax"), "");
		rt( P::Numeric, "maxdf", _TXT("the maximum df for a feature to be not considered as stopword and not subject for proximity weighting as fraction of the collection size"), "0:");
		rt( P::Numeric, "results", _TXT( "maximum number of weighted elements returned by each collect configuration"), "1:");
		rt( P::Numeric, "dist_imm", _TXT( "ordinal position distance considered as immediate in the same sentence"), "1:");
		rt( P::Numeric, "dist_close", _TXT( "ordinal position distance considered as close in the same sentence"), "1:");
		rt( P::Numeric, "dist_near", _TXT( "ordinal position distance considered as near for features not in the same sentence"), "1:");
		rt( P::Numeric, "dist_collect", _TXT( "search distance for neighbour elements to collect"), "1:");
		rt( P::Numeric, "cluster", _TXT( "part [0.0,1.0] of query features considered as relevant in a group"), "1:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

