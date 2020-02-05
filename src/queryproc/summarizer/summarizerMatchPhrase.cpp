/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMatchPhrase.hpp"
#include "proximityWeightAccumulator.hpp"
#include "positionWindow.hpp"
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
			m_itrar[ m_itrarsize] = itr;
			m_weightar[ m_itrarsize] = idf * weight;
			++m_itrarsize;
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
		if (m_itrarsize == 0)
		{
			return std::vector<SummaryElement>();
		}
		if (m_itrarsize == 1)
		{
			double typedWordFraction( const strus::IndexRange& field);
		}
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

static std::pair<std::string,std::string> parseMarker( const std::string& value)
{
	std::pair<std::string,std::string> rt;
	if (!value.empty())
	{
		char sep = value[0];
		char const* mid = std::strchr( value.c_str()+1, sep);
		if (mid)
		{
			rt.first = std::string( value.c_str()+1, mid - value.c_str() - 1);
			rt.second = std::string( mid+1);
		}
		else
		{
			rt.first = value;
			rt.second = value;
		}
	}
	return rt;
}

void SummarizerFunctionInstanceMatchPhrase::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "punct") || strus::caseInsensitiveEquals( name_, "para") || strus::caseInsensitiveEquals( name_, "title"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name_.c_str(), "matchvariables");
		}
		else if (strus::caseInsensitiveEquals( name_, "type"))
		{
			m_parameter->m_type = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "matchmark"))
		{
			m_parameter->m_matchmark = parseMarker( value);
		}
		else if (strus::caseInsensitiveEquals( name_, "floatingmark"))
		{
			m_parameter->m_floatingmark = parseMarker( value);
		}
		else if (strus::caseInsensitiveEquals( name_, "sentencesize")
			|| strus::caseInsensitiveEquals( name_, "paragraphsize")
			|| strus::caseInsensitiveEquals( name_, "windowsize"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no string value expected for parameter '%s' in summarization function '%s'"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "cardinality") && !value.empty() && value[value.size()-1] == '%')
		{
			m_parameter->m_cardinality = 0;
			m_parameter->m_cardinality_frac = numstring_conv::todouble( value);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchPhrase::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match") || strus::caseInsensitiveEquals( name_, "punct") || strus::caseInsensitiveEquals( name_, "para") || strus::caseInsensitiveEquals( name_, "title"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "paragraphsize"))
	{
		m_parameter->m_paragraphsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "sentencesize"))
	{
		m_parameter->m_sentencesize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "windowsize"))
	{
		m_parameter->m_windowsize = value.touint();
	}
	else if (strus::caseInsensitiveEquals( name_, "cardinality"))
	{
		m_parameter->m_cardinality = value.touint();
		m_parameter->m_cardinality_frac = 0.0;
	}
	else if (strus::caseInsensitiveEquals( name_, "maxdf"))
	{
		m_parameter->m_maxdf = (double)value;
		if (m_parameter->m_maxdf < 0.0 || m_parameter->m_maxdf > 1.0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer function '%s' expected to a positive floating point number between 0.0 and 1.0"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	else if (strus::caseInsensitiveEquals( name_, "type")
		|| strus::caseInsensitiveEquals( name_, "matchmark")
		|| strus::caseInsensitiveEquals( name_, "floatingmark")
		|| strus::caseInsensitiveEquals( name_, "metadata_title_maxpos"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name_.c_str(), THIS_METHOD_NAME);
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
	if (m_parameter->m_type.empty())
	{
		m_errorhnd->report( ErrorCodeIncompleteDefinition, _TXT( "emtpy term type definition (parameter 'type') in match phrase summarizer configuration"));
	}
	try
	{
		strus::Reference<MetaDataReaderInterface> metadata( storage->createMetaDataReader());
		if (!metadata.get()) throw strus::runtime_error(_TXT("failed to create meta data reader: %s"), m_errorhnd->fetchError());

		double nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new SummarizerFunctionContextMatchPhrase(
				storage, m_processor, metadata.release(), m_parameter,
				nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

static StructView getStructViewMark( const std::pair<std::string,std::string>& mark)
{
	return StructView()( "start", mark.first)( "end", mark.second);
}

StructView SummarizerFunctionInstanceMatchPhrase::view() const
{
	try
	{
		StructView rt;
		rt( "type", m_parameter->m_type);
		rt( "matchmark", getStructViewMark( m_parameter->m_matchmark));
		rt( "floatingmark", getStructViewMark( m_parameter->m_floatingmark));
		rt( "paragraphsize", m_parameter->m_paragraphsize);
		rt( "sentencesize", m_parameter->m_sentencesize);
		rt( "windowsize", m_parameter->m_windowsize);
		if (m_parameter->m_cardinality_frac > std::numeric_limits<float>::epsilon())
		{
			rt( "cardinality", strus::string_format( "%u%%", (unsigned int)(m_parameter->m_cardinality_frac * 100 + 0.5)));
		}
		else
		{
			rt( "cardinality", m_parameter->m_cardinality);
		}
		rt( "maxdf", m_parameter->m_maxdf);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionInstanceInterface* SummarizerFunctionMatchPhrase::createInstance(
		const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceMatchPhrase( processor, m_errorhnd);
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
		rt( P::Feature, "para", _TXT( "defines the delimiter for paragraphs (summaries must not overlap paragraph borders)"), "");
		rt( P::Feature, "title", _TXT( "defines the title field of documents"), "");
		rt( P::String, "type", _TXT( "the forward index type of the result phrase elements"), "");
		rt( P::Numeric, "paragraphsize", _TXT( "estimated size of a paragraph"), "1:");
		rt( P::Numeric, "sentencesize", _TXT( "estimated size of a sentence, also a restriction for the maximum length of sentences in summaries"), "1:");
		rt( P::Numeric, "windowsize", _TXT( "maximum size of window used for identifying matches"), "1:");
		rt( P::Numeric, "cardinality", _TXT( "minimum number of features in a window"), "1:");
		rt( P::Numeric, "maxdf", _TXT( "the maximum df (fraction of collection size) of features considered for same sentence proximity weighing"), "1:");
		rt( P::String, "matchmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for highlighting matches in the resulting phrases"), "");
		rt( P::String, "floatingmark", _TXT( "specifies the markers (first character of the value is the separator followed by the two parts separated by it) for marking floating phrases without start or end of sentence found"), "");
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


