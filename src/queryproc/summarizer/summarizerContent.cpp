/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerContent.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <set>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("content")

SummarizerFunctionContextContent::SummarizerFunctionContextContent( const StorageClientInterface* storage_, const std::string& type_, unsigned int maxNofMatches_, ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_forwardindex(storage_->createForwardIterator(type_))
	,m_type(type_),m_maxNofMatches(maxNofMatches_),m_errorhnd(errorhnd_)
{
	if (!m_forwardindex.get())
	{
		throw strus::runtime_error(_TXT("failed to create summarizer context for '%s': %s"), THIS_METHOD_NAME, m_errorhnd->fetchError());
	}
}

void SummarizerFunctionContextContent::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextContent::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		double /*weight*/)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no summarization feature defined for '%s'"), THIS_METHOD_NAME);
}

std::vector<SummaryElement>
	SummarizerFunctionContextContent::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		unsigned int cnt = m_maxNofMatches;
		m_forwardindex->skipDoc( doc.docno());
		strus::Index pos = m_forwardindex->skipPos( doc.field().start());
		strus::Index endpos = doc.field().defined() ? doc.field().end() : std::numeric_limits<strus::Index>::max();
		for (; pos && pos < endpos; pos = m_forwardindex->skipPos( pos+1))
		{
			rt.push_back( SummaryElement( m_type, m_forwardindex->fetch(), 1.0, pos));
			if (--cnt == 0) break;
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

void SummarizerFunctionInstanceContent::addStringParameter( const std::string& name_, const std::string& value)
{
	if (strus::caseInsensitiveEquals( name_, "type"))
	{
		m_type = value;
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

void SummarizerFunctionInstanceContent::addNumericParameter( const std::string& name_, const NumericVariant& val)
{
	if (strus::caseInsensitiveEquals( name_, "type"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "results"))
	{
		m_maxNofMatches = std::min(
			val.touint(),
			(NumericVariant::UIntType)std::numeric_limits<unsigned int>::max());
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}


SummarizerFunctionContextInterface* SummarizerFunctionInstanceContent::createFunctionContext(
		const StorageClientInterface* storage_,
		const GlobalStatistics&) const
{
	try
	{
		if (m_type.empty())
		{
			throw strus::runtime_error(_TXT("missing type and result definition of '%s'"), THIS_METHOD_NAME);
		}
		return new SummarizerFunctionContextContent( storage_, m_type, m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceContent::view() const
{
	try
	{
		StructView rt;
		rt( "type", m_type);
		rt( "results", m_maxNofMatches);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionContent::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceContent( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


StructView SummarizerFunctionContent::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Get the complete forward index"));
		rt( P::String, "type", _TXT( "the forward index type to fetch the summary elements"), "");
		rt( P::Numeric, "results", _TXT( "the maximum number of matches to return"), "1:");
		return std::move(rt);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* SummarizerFunctionInstanceContent::name() const
{
	return THIS_METHOD_NAME;
}

const char* SummarizerFunctionContent::name() const
{
	return THIS_METHOD_NAME;
}

