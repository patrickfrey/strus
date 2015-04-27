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
#ifndef _STRUS_SUMMARIZER_METADATA_HPP_INCLUDED
#define _STRUS_SUMMARIZER_METADATA_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerExecutionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class QueryProcessorInterface;


/// \brief Interface for the summarization context (of a SummarizationFunction)
class SummarizerExecutionContextMetaData
	:public SummarizerExecutionContextInterface
{
public:
	/// \brief Constructor
	/// \param[in] metadata_ reader for document meta data
	/// \param[in] name_ meta data field identifier
	SummarizerExecutionContextMetaData( MetaDataReaderInterface* metadata_, const std::string& name_);

	virtual ~SummarizerExecutionContextMetaData(){}

	virtual void addSummarizationFeature(
			const std::string&,
			PostingIteratorInterface*,
			const std::vector<SummarizationVariable>&)
	{
		throw strus::runtime_error( _TXT( "no sumarization features expected in summarization function '%s'"), "MetaData");
	}

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	MetaDataReaderInterface* m_metadata;
	int m_attrib;
};


/// \class SummarizerFunctionInstanceMetaData
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMetaData
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceMetaData()
		:m_name(){}

	virtual ~SummarizerFunctionInstanceMetaData(){}

	virtual void addStringParameter( const std::string& name, const std::string& value)
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			m_name = value;
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MetaData", name.c_str());
		}
	}

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)
	{
		if (utils::caseInsensitiveEquals( name, "name"))
		{
			throw strus::runtime_error( _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name.c_str(), "MetaData");
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), "MetaData", name.c_str());
		}
	}

	virtual SummarizerExecutionContextInterface* createExecutionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface* metadata) const
	{
		return new SummarizerExecutionContextMetaData( metadata, m_name);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << "name='" << m_name << "'";
		return rt.str();
	}

private:
	std::string m_name;
};


class SummarizerFunctionMetaData
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMetaData(){}

	virtual ~SummarizerFunctionMetaData(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const
	{
		return new SummarizerFunctionInstanceMetaData();
	}
};

}//namespace
#endif


