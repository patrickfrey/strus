/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMatchPhrase.hpp"
#include "postingIteratorLink.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("matchphrase")

SummarizerFunctionContextMatchPhrase::SummarizerFunctionContextMatchPhrase(
		const StorageClientInterface* storage_,
		const SummarizerFunctionParameterMatchPhrase& parameter_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_proximityWeightingContext(parameter_.proximityConfig)
	,m_parameter(parameter_)
	,m_itrarsize(0)
	,m_weightar()
	,m_eos_itr(0)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_storage(storage_)
	,m_textCollector( storage_, parameter_.contentType, parameter_.entityType, errorhnd_)
	,m_errorhnd(errorhnd_)
{
	std::memset( &m_itrar, 0, sizeof(m_itrar));
}

void SummarizerFunctionContextMatchPhrase::setVariableValue( const std::string& name_, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextMatchPhrase::addSummarizationFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
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
			if (m_itrarsize >= MaxNofArguments) throw std::runtime_error( _TXT("number of weighting features out of range"));
			if (m_parameter.maxdf * m_nofCollectionDocuments < df)
			{
				//... stopwords are ignored
			}
			else
			{
				m_itrar[ m_itrarsize] = itr;
				m_weightar[ m_itrarsize] = idf * weight;
				++m_itrarsize;
			}
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarizer function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

SummarizerFunctionContextMatchPhrase::~SummarizerFunctionContextMatchPhrase()
{}

std::vector<SummaryElement>
	SummarizerFunctionContextMatchPhrase::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		strus::WeightedField bestMatch;

		if (m_itrarsize == 0)
		{
			return rt;
		}
		else if (m_itrarsize == 1)
		{
			if (doc.docno() == m_itrar[0]->skipDoc( doc.docno()))
			{
				strus::Index startpos = doc.field().defined() ? doc.field().start() : 1;
				strus::Index endpos = doc.field().defined() ? doc.field().end() : std::numeric_limits<strus::Index>::max();
				if (m_eos_itr)
				{
					SentenceIterator sentiter( m_eos_itr, doc.docno(), doc.field(), m_parameter.proximityConfig.maxNofSummarySentenceWords);
					strus::Index pos = m_itrar[0]->skipPos( startpos);
					while (pos)
					{
						strus::IndexRange sent = sentiter.skipPos( pos);
						if (!sent.defined()) break;
						if (sent.contain(pos))
						{
							bestMatch = strus::WeightedField( sent, 1.0);
							break;
						}
						else
						{
							pos = m_itrar[0]->skipPos( sent.end());
						}
					}
				}
				else
				{
					strus::Index pos = m_itrar[0]->skipPos( startpos);
					if (pos && pos < endpos)
					{
						strus::Index dist = m_parameter.proximityConfig.nofSummarySentences
									* m_parameter.proximityConfig.maxNofSummarySentenceWords
									/ 2;
						if (pos > (startpos + dist)) startpos = (pos - dist);
						if ((pos + dist) < endpos) endpos = (pos + dist);
						bestMatch = strus::WeightedField( 
								strus::IndexRange( startpos, endpos), 1.0);
					}
				}
			}
		}
		else
		{
			m_proximityWeightingContext.init( m_itrar, m_itrarsize, m_eos_itr, doc.docno(), doc.field());
			bestMatch = m_proximityWeightingContext.getBestPassage( m_weightar);
		}
		if (bestMatch.field().defined())
		{
			m_textCollector.skipDoc( doc.docno());
			rt.push_back( SummaryElement( "", m_textCollector.fetch( bestMatch.field()), bestMatch.weight()));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void SummarizerFunctionInstanceMatchPhrase::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "punct"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a feature and not as a string"), name_.c_str(), "matchvariables");
		}
		else if (strus::caseInsensitiveEquals( name_, "text"))
		{
			m_parameter.contentType = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "entity"))
		{
			m_parameter.entityType = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "dist_imm")
			|| strus::caseInsensitiveEquals( name_, "dist_close")
			|| strus::caseInsensitiveEquals( name_, "dist_near")
			|| strus::caseInsensitiveEquals( name_, "dist_sentence")
			|| strus::caseInsensitiveEquals( name_, "sentences")
			|| strus::caseInsensitiveEquals( name_, "cluster")
			|| strus::caseInsensitiveEquals( name_, "ffbase")
			|| strus::caseInsensitiveEquals( name_, "maxdf")
			)
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "punct"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a feature and not as a numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "text")
		|| strus::caseInsensitiveEquals( name_, "tag")
		|| strus::caseInsensitiveEquals( name_, "entity"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a string"), name_.c_str(), THIS_METHOD_NAME);
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
	else if (strus::caseInsensitiveEquals( name_, "dist_sentence"))
	{
		m_parameter.proximityConfig.setMaxNofSummarySentenceWords( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "sentences"))
	{
		m_parameter.proximityConfig.setNofSummarySentences( value.toint());
	}
	else if (strus::caseInsensitiveEquals( name_, "cluster"))
	{
		m_parameter.proximityConfig.setMinClusterSize( value.tofloat());
	}
	else if (strus::caseInsensitiveEquals( name_, "ffbase"))
	{
		m_parameter.proximityConfig.setMinFfWeight( value.tofloat());
	}
	else if (strus::caseInsensitiveEquals( name_, "maxdf"))
	{
		m_parameter.maxdf = value.tofloat();
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceMatchPhrase::createFunctionContext(
		const StorageClientInterface* storage,
		const GlobalStatistics& stats) const
{
	if (m_parameter.contentType.empty())
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "emtpy content type definition (parameter '%s') in match phrase summarizer configuration"), "text");
	}
	try
	{
		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextMatchPhrase(
				storage, m_parameter, nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceMatchPhrase::view() const
{
	try
	{
		StructView rt;
		rt( "text", m_parameter.contentType);
		rt( "entity", m_parameter.entityType);
		rt( "maxdf", m_parameter.maxdf);
		rt( "dist_imm", m_parameter.proximityConfig.distance_imm);
		rt( "dist_close", m_parameter.proximityConfig.distance_close);
		rt( "dist_near", m_parameter.proximityConfig.distance_near);
		rt( "dist_sentence", m_parameter.proximityConfig.maxNofSummarySentenceWords);
		rt( "sentences", m_parameter.proximityConfig.nofSummarySentences);
		rt( "cluster", m_parameter.proximityConfig.minClusterSize);
		rt( "ffbase", m_parameter.proximityConfig.minFfWeight);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionInstanceInterface* SummarizerFunctionMatchPhrase::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchPhrase( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionMatchPhrase::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Get best matching phrases delimited by the structure postings"));
		rt( P::Feature, "match", _TXT( "defines the features to weight"), "");
		rt( P::Feature, "punct", _TXT( "defines the delimiter for sentences"), "");
		rt( P::String, "text", _TXT( "the forward index type of the result phrase elements"), "");
		rt( P::String, "entity", _TXT( "the forward index type of the entities in the document"), "");
		rt( P::Numeric, "dist_imm", _TXT( "ordinal position distance considered as immediate in the same sentence"), "1:");
		rt( P::Numeric, "dist_close", _TXT( "ordinal position distance considered as close in the same sentence"), "1:");
		rt( P::Numeric, "dist_near", _TXT( "ordinal position distance considered as near for features not in the same sentence"), "1:");
		rt( P::Numeric, "dist_sentence", _TXT( "maximal size of a sentence"), "1:");
		rt( P::Numeric, "sentences", _TXT( "number of sentences extracted for the best match"), "1:");
		rt( P::Numeric, "cluster", _TXT( "part [0.0,1.0] of query features considered as relevant in a group"), "1:");
		rt( P::Numeric, "ffbase", _TXT( "value in the range from 0.0 to 1.0 specifying the minimum feature occurrence value assigned to a feature"), "0.0:1.0");
		rt( P::Numeric, "maxdf", _TXT("the maximum df for a feature to be not considered as stopword as fraction of the collection size"), "0:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* SummarizerFunctionInstanceMatchPhrase::name() const
{
	return THIS_METHOD_NAME;
}
const char* SummarizerFunctionMatchPhrase::name() const
{
	return THIS_METHOD_NAME;
}


