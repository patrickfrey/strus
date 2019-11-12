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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextListMatches::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << string_format( _TXT( "summarize %s"), THIS_METHOD_NAME) << std::endl;

	std::vector<SummaryElement> res = getSummary( docno);
	std::vector<SummaryElement>::const_iterator ri = res.begin(), re = res.end();
	for (; ri != re; ++ri)
	{
		out << string_format( _TXT("match %s %s"), ri->name().c_str(), ri->value().c_str()) << std::endl;
	}
	return out.str();
}


void SummarizerFunctionInstanceListMatches::addStringParameter( const std::string& name_, const std::string& value)
{
	if (strus::caseInsensitiveEquals( name_, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "name"))
	{
		m_resultname = value;
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
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
	}
	else if (strus::caseInsensitiveEquals( name_, "name"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as string and not as numeric value"), name_.c_str(), THIS_METHOD_NAME);
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

void SummarizerFunctionInstanceListMatches::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (strus::caseInsensitiveEquals( itemname, "position"))
		{
			m_resultname = resultname;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), "ListMatches", *m_errorhnd);
}


SummarizerFunctionContextInterface* SummarizerFunctionInstanceListMatches::createFunctionContext(
		const StorageClientInterface*,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextListMatches( m_resultname, m_maxNofMatches, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceListMatches::view() const
{
	try
	{
		StructView rt;
		rt( "nof", m_maxNofMatches);
		if (!m_resultname.empty()) rt( "result", m_resultname);
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


