/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
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
#include "private/internationalization.hpp"
#include "private/utils.hpp"

using namespace strus;

SummarizerExecutionContextAttribute::SummarizerExecutionContextAttribute(
		AttributeReaderInterface* attribreader_, const std::string& name_)
	:m_attribreader(attribreader_)
	,m_attrib(attribreader_->elementHandle( name_.c_str()))
{}

void SummarizerExecutionContextAttribute::addSummarizationFeature(
		const std::string&,
		PostingIteratorInterface*,
		const std::vector<SummarizationVariable>&)
{
	throw strus::runtime_error( _TXT( "no sumarization features expected in summarization function '%s'"), "MetaData");
}

SummarizerExecutionContextAttribute::~SummarizerExecutionContextAttribute()
{
	delete m_attribreader;
}

std::vector<SummarizerExecutionContextInterface::SummaryElement>
	SummarizerExecutionContextAttribute::getSummary( const Index& docno)
{
	std::vector<SummarizerExecutionContextInterface::SummaryElement> rt;
	m_attribreader->skipDoc( docno);
	std::string attr = m_attribreader->getValue( m_attrib);
	if (!attr.empty()) 
	{
		rt.push_back( SummarizerExecutionContextInterface::SummaryElement( attr, 1.0));
	}
	return rt;
}


void SummarizerFunctionInstanceAttribute::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		m_name = value;
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "Attribute", name.c_str());
	}
}

void SummarizerFunctionInstanceAttribute::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "name"))
	{
		throw strus::runtime_error( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "Attribute");
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "Attribute", name.c_str());
	}
}

bool SummarizerFunctionInstanceAttribute::isFeatureParameter( const std::string&) const
{
	return false;
}

SummarizerExecutionContextInterface* SummarizerFunctionInstanceAttribute::createExecutionContext(
		const StorageClientInterface* storage,
		MetaDataReaderInterface*) const
{
	return new SummarizerExecutionContextAttribute( storage->createAttributeReader(), m_name);
}


