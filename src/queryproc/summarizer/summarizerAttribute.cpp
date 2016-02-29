/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "summarizerAttribute.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"

using namespace strus;

SummarizerFunctionContextAttribute::SummarizerFunctionContextAttribute(
		AttributeReaderInterface* attribreader_, const std::string& name_, ErrorBufferInterface* errorhnd_)
	:m_attribreader(attribreader_)
	,m_attribname(name_)
	,m_attrib(attribreader_->elementHandle( name_.c_str()))
	,m_errorhnd(errorhnd_)
{
	if (!m_attrib)
	{
		throw strus::runtime_error(_TXT("unknown attribute name '%s' passed to summarizer '%s'"), m_attribname.c_str(), "attribute");
	}
}

void SummarizerFunctionContextAttribute::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&,
		float /*weight*/,
		const TermStatistics&)
{
	m_errorhnd->report( _TXT( "no sumarization features expected in summarization function '%s'"), "attribute");
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
			rt.push_back( SummaryElement( m_attribname, attr, 1.0));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), "attribute", *m_errorhnd, std::vector<SummaryElement>());
}


void SummarizerFunctionInstanceAttribute::addStringParameter( const std::string& name, const std::string& value)
{
	try
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			m_name = value;
		}
		else
		{
			m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "Attribute", name.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), "attribute", *m_errorhnd);
}

void SummarizerFunctionInstanceAttribute::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_errorhnd->report( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "Attribute");
	}
	else
	{
		m_errorhnd->report( _TXT("unknown '%s' summarization function parameter '%s'"), "Attribute", name.c_str());
	}
}

std::string SummarizerFunctionInstanceAttribute::tostring() const
{
	try
	{
		std::ostringstream rt;
		rt << "name='" << m_name << "'";
		return rt.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), "attribute", *m_errorhnd, std::string());
}


SummarizerFunctionContextInterface* SummarizerFunctionInstanceAttribute::createFunctionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*,
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
		return new SummarizerFunctionContextAttribute( reader, m_name, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), "attribute", *m_errorhnd, 0);
}


SummarizerFunctionInstanceInterface* SummarizerFunctionAttribute::createInstance(
		const QueryProcessorInterface*) const
{
	try
	{
		return new SummarizerFunctionInstanceAttribute( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), "attribute", *m_errorhnd, 0);
}


SummarizerFunctionInterface::Description SummarizerFunctionAttribute::getDescription() const
{
	try
	{
		Description rt( _TXT("Get the value of a document attribute."));
		rt( Description::Param::Attribute, "name", _TXT( "the name of the attribute to get"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), "attribute", *m_errorhnd, SummarizerFunctionInterface::Description());
}

