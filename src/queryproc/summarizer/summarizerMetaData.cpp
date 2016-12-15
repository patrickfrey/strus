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
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

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
		throw strus::runtime_error(_TXT("unknown metadata element name '%s' passed to summarizer '%s'"), m_metaname.c_str(), "metadata");
	}
}

void SummarizerFunctionContextMetaData::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	m_errorhnd->report( _TXT( "no sumarization features expected in summarization function '%s'"), "MetaData");
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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "metadata", *m_errorhnd, std::vector<SummaryElement>());
}


void SummarizerFunctionInstanceMetaData::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			m_metaname = value;
			m_resultname = value;
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MetaData", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "metadata", *m_errorhnd);
}

void SummarizerFunctionInstanceMetaData::defineResultName(
		const std::string& resultname,
		const std::string& itemname)
{
	try
	{
		if (utils::caseInsensitiveEquals( itemname, "metaname"))
		{
			m_metaname = resultname;
		}
		else if (utils::caseInsensitiveEquals( itemname, "resultname"))
		{
			m_resultname = resultname;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), "metadata", *m_errorhnd);
}

void SummarizerFunctionInstanceMetaData::addNumericParameter( const std::string& name, const NumericVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MetaData");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "MetaData", name.c_str());
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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "metadata", *m_errorhnd, 0);
}

std::string SummarizerFunctionInstanceMetaData::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "metaname='" << m_metaname << "', resultname='" << m_resultname << "'";
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "metadata", *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionMetaData::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceMetaData( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "metadata", *m_errorhnd, 0);
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
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "metadata", *m_errorhnd, FunctionDescription());
}

