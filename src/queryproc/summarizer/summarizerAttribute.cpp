/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerAttribute.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("attribute")

SummarizerFunctionContextAttribute::SummarizerFunctionContextAttribute(
		AttributeReaderInterface* attribreader_, const std::string& attribname_, const std::string& resultname_, ErrorBufferInterface* errorhnd_)
	:m_attribreader(attribreader_)
	,m_attribname(attribname_)
	,m_resultname(resultname_)
	,m_attrib(attribreader_->elementHandle( attribname_.c_str()))
	,m_errorhnd(errorhnd_)
{
	if (!m_attrib)
	{
		throw strus::runtime_error(_TXT("unknown attribute name '%s' passed to summarizer '%s'"), m_attribname.c_str(), THIS_METHOD_NAME);
	}
}

void SummarizerFunctionContextAttribute::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

void SummarizerFunctionContextAttribute::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		double /*weight*/,
		const TermStatistics&)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT( "no sumarization features expected in summarization function '%s'"), THIS_METHOD_NAME);
}

SummarizerFunctionContextAttribute::~SummarizerFunctionContextAttribute()
{
	delete m_attribreader;
}

std::vector<SummaryElement>
	SummarizerFunctionContextAttribute::getSummary( const Index& docno)
{
	try
	{
		std::vector<SummaryElement> rt;
		m_attribreader->skipDoc( docno);
		std::string attr = m_attribreader->getValue( m_attrib);
		if (!attr.empty()) 
		{
			rt.push_back( SummaryElement( m_resultname, attr, 1.0));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

std::string SummarizerFunctionContextAttribute::debugCall( const Index& docno)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(8);
	out << strus::string_format( _TXT( "summarize %s"), THIS_METHOD_NAME) << std::endl;

	m_attribreader->skipDoc( docno);
	std::string attr = m_attribreader->getValue( m_attrib);
	if (!attr.empty()) 
	{
		out << strus::string_format( _TXT( "attribute name=%s, value=%s"),
				m_resultname.c_str(), attr.c_str()) << std::endl;
	}
	return out.str();
}

void SummarizerFunctionInstanceAttribute::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "name"))
		{
			m_resultname = value;
			m_attribname = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "attribute"))
		{
			m_attribname = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "result"))
		{
			m_resultname = value;
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceAttribute::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "name"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name_.c_str(), THIS_METHOD_NAME);
	}
	else
	{
		m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
	}
}

void SummarizerFunctionInstanceAttribute::defineResultName(
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
	CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), "metadata", *m_errorhnd);
}

StructView SummarizerFunctionInstanceAttribute::view() const
{
	try
	{
		StructView rt;
		rt( "attribute", m_attribname);
		if (!m_resultname.empty()) rt( "result", m_resultname);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceAttribute::createFunctionContext(
		const StorageClientInterface* storage,
		const GlobalStatistics&) const
{
	try
	{
		AttributeReaderInterface* reader = storage->createAttributeReader();
		if (!reader)
		{
			m_errorhnd->explain( _TXT("error creating context of 'attribute' summarizer: %s"));
			return 0;
		}
		return new SummarizerFunctionContextAttribute( reader, m_attribname, m_resultname.empty()? m_attribname : m_resultname, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAttribute::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceAttribute( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}


StructView SummarizerFunctionAttribute::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Get the value of a document attribute."));
		rt( P::Attribute, "name", _TXT( "the name of the attribute to get"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

