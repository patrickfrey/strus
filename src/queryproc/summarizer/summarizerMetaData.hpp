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
#include "strus/summarizerClosureInterface.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class QueryProcessorInterface;


/// \brief Interface for the summarization context (of a SummarizationFunction)
class SummarizerClosureMetaData
	:public SummarizerClosureInterface
{
public:
	/// \param[in] metadata_ reader for document meta data
	/// \param[in] name_ meta data field identifier
	SummarizerClosureMetaData( MetaDataReaderInterface* metadata_, const std::string& name_);
	virtual ~SummarizerClosureMetaData(){}

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	MetaDataReaderInterface* m_metadata;
	int m_attrib;
};


class SummarizerFunctionMetaData
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMetaData(){}

	virtual ~SummarizerFunctionMetaData(){}

	virtual const char** textualParameterNames() const
	{
		static const char* ar[] = {"name",0};
		return ar;
	}

	virtual SummarizerClosureInterface* createClosure(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			MetaDataReaderInterface* metadata_,
			const std::vector<FeatureParameter>& features_,
			const std::vector<std::string>& textualParameters_,
			const std::vector<ArithmeticVariant>& numericParameters_) const
	{
		return new SummarizerClosureMetaData( metadata_, textualParameters_[0]);
	}
};

}//namespace
#endif


