/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMetaData.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define METHOD_NAME "BM25"

SummarizerFunctionContextMetaData::SummarizerFunctionContextMetaData( 
		MetaDataReaderInterface* metadata_, const std::string& metaname_, const std::string& resultname_, ErrorBufferInterface* errorhnd_)
	:m_metadata(metadata_)
	,m_resultname(resultname_)
	,m_metaname(metaname_)
	,m_attrib(metadata_->elementHandle( metaname_.c_str()))
	,m_errorhnd(errorhnd_)
{
	if (m_attrib < 0)
	{
		throw strus::runtime_error( _TXT("unknown metadata element name '%s' passed to summarizer '%s'"), m_metaname.c_str(), METHOD_NAME);
	}
}

void SummarizerFunctionContextMetaData::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseNotImplemented), _TXT("no variables known for function '%s'"), METHOD_NAME);
}

void SummarizerFunctionContextMetaData::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidOperation), _TXT( "no sumarization features expected in summarization function '%s'"), METHOD_NAME);
}

std::vector<SummaryElement>
	SummarizerFunctionContextMetaData::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		m_metadata->skipDoc( docno);
		NumericVariant value = m_metadata->getValue( m_attrib);
		if (value.defined()) 
		{
			rt.push_back( SummaryElement( m_resultname, value.tostring().c_str(), 1.0));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextMetaData::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << string_format( _TXT( "summarize %s"), METHOD_NAME) << std::endl;

	m_metadata->skipDoc( docno);
	NumericVariant value = m_metadata->getValue( m_attrib);
	if (value.defined()) 
	{
		out << string_format( _TXT( "metadata name=%s, value=%s"), m_resultname.c_str(), value.tostring().c_str()) << std::endl;
	}
	return out.str();
}

void SummarizerFunctionInstanceMetaData::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name, "name"))
		{
			m_metaname = value;
			m_resultname = value;
		}
		else if (strus::caseInsensitiveEquals( name, "element"))
		{
			m_metaname = value;
		}
		else if (strus::caseInsensitiveEquals( name, "result"))
		{
			m_resultname = value;
		}
		else
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMetaData::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (strus::caseInsensitiveEquals( itemname, m_resultname) || strus::caseInsensitiveEquals( itemname, "result"))
		{
			m_resultname = resultname;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceMetaData::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name, "name"))
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseInvalidArgument), _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationBuildData,ErrorCauseUnknownIdentifier), _TXT("unknown '%s' summarization function parameter '%s'"), METHOD_NAME, name.c_str());
	}
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceMetaData::createFunctionContext(
		const StorageClientInterface*,
		MetaDataReaderInterface* metadata,
		const GlobalStatistics&) const
{
	try
	{
		return new SummarizerFunctionContextMetaData( metadata, m_metaname, m_resultname, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMetaData::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "metaname='" << m_metaname << "', resultname='" << m_resultname << "'";
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMetaData::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceMetaData( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), METHOD_NAME, *m_errorhnd, 0);
}



FunctionDescription SummarizerFunctionMetaData::getDescription() const
{
	try
	{
		typedef FunctionDescription::Parameter P;
		FunctionDescription rt( _TXT("Get the value of a document meta data element."));
		rt( P::Metadata, "name", _TXT( "the name of the meta data element to get"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), METHOD_NAME, *m_errorhnd, FunctionDescription());
}

