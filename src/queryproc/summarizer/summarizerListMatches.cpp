/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerListMatches.hpp"
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

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("matchpos")

void SummarizerFunctionContextListMatches::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextListMatches::addSummarizationFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			m_itrs.push_back( itr);
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization feature '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding summarization feature to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

std::vector<SummaryElement>
	SummarizerFunctionContextListMatches::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		std::vector<PostingIteratorInterface*>::const_iterator
			ii = m_itrs.begin(), ie = m_itrs.end();
	
		for (; ii != ie; ++ii)
		{
			if ((*ii)->skipDoc( doc.docno()) == doc.docno())
			{
				unsigned int kk=0;
				strus::Index pos = (*ii)->skipPos( doc.field().start());
				strus::Index endpos = doc.field().defined() ? doc.field().end() : std::numeric_limits<strus::Index>::max();

				for (int gidx=0; pos && pos < endpos && kk<m_maxNofMatches; ++kk,pos = (*ii)->skipPos( pos+1))
				{
					char posstr[ 64];
					snprintf( posstr, sizeof(posstr), "%u", (unsigned int)pos);
					rt.push_back( SummaryElement( "", posstr, 1.0, gidx));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name_, const std::string& value)
{
	if (strus::caseInsensitiveEquals( name_, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a feature and not as a string"), name_.c_str(), THIS_METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

void SummarizerFunctionInstanceListMatches::addNumericParameter( const std::string& name_, const NumericVariant& val)
{
	if (strus::caseInsensitiveEquals( name_, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a feature and not as a numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "name"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as a string and not as a numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "N"))
	{
		m_maxNofMatches = val.touint();
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceListMatches::view() const
{
	try
	{
		StructView rt;
		rt( "nof", m_maxNofMatches);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionInstanceInterface* SummarizerFunctionListMatches::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceListMatches( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


StructView SummarizerFunctionListMatches::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Get the feature occurencies printed"));
		rt( P::Feature, "match", _TXT( "defines the query features"), "");
		rt( P::Numeric, "N", _TXT( "the maximum number of matches to return"), "1:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* SummarizerFunctionInstanceListMatches::name() const
{
	return THIS_METHOD_NAME;
}

const char* SummarizerFunctionListMatches::name() const
{
	return THIS_METHOD_NAME;
}


