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
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include <set>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace strus;

void SummarizerFunctionContextListMatches::addSummarizationFeature(
		const std::string& name,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "match"))
		{
			m_itrs.push_back( itr);
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization feature '%s'"), "matchpos", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding summarization feature to '%s' summarizer: %s"), "matchpos", *m_errorhnd);
}

std::vector<SummaryElement>
	SummarizerFunctionContextListMatches::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		std::vector<PostingIteratorInterface*>::const_iterator
			ii = m_itrs.begin(), ie = m_itrs.end();
	
		for (; ii != ie; ++ii)
		{
			if ((*ii)->skipDoc( docno) == docno)
			{
				unsigned int kk=0;
				Index pos = (*ii)->skipPos( 0);
				for (int gidx=0; pos && kk<m_maxNofMatches; ++kk,pos = (*ii)->skipPos( pos+1))
				{
					char posstr[ 64];
					snprintf( posstr, sizeof(posstr), "%u", (unsigned int)pos);
					rt.push_back( SummaryElement( m_resultname, posstr, 1.0, gidx));
				}
			}
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "matchpos", *m_errorhnd, std::vector<SummaryElement>());
}

void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name.c_str(), "matchpos");
	}
	else if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_resultname = value;
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "matchpos", name.c_str());
	}
}

void SummarizerFunctionInstanceListMatches::addNumericParameter( const std::string& name, const NumericVariant& val)
{
	if (utils::caseInsensitiveEquals( name, "match"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name.c_str(), "matchpos");
	}
	else if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_errorhnd->report( _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name.c_str(), "matchpos");
	}
	else if (utils::caseInsensitiveEquals( name, "N"))
	{
		m_maxNofMatches = val.touint();
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "matchpos", name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( m_resultname, m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "matchpos", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceListMatches::tostring() const
{
	return std::string();
}


SummarizerFunctionInstanceInterface* SummarizerFunctionListMatches::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceListMatches( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "matchpos", *m_errorhnd, 0);
}


FunctionDescription SummarizerFunctionListMatches::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Get the feature occurencies printed"));
		rt( P::Feature, "match", _TXT( "defines the query features"), "");
		rt( P::Numeric, "N", _TXT( "the maximum number of matches to return"), "1:");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "matchpos", *m_errorhnd, FunctionDescription());
}


