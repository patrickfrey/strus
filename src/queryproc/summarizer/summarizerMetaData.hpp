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
class MetaDataReaderInterface;


/// \brief Interface for the summarization context (of a SummarizationFunction)
class SummarizerClosureMetaData
	:public SummarizerClosureInterface
{
public:
	SummarizerClosureMetaData( MetaDataReaderInterface* metadata_, const char* name_);
	virtual ~SummarizerClosureMetaData(){}

	virtual std::vector<std::string> getSummary( const Index& docno);

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

	virtual const char* name() const
	{
		return "metadata";
	}

	virtual const char** parameterNames() const
	{
		static const char* ar[] = {0};
		return ar;
	}

	virtual SummarizerClosureInterface* createClosure(
			const StorageInterface*,
			const char* elementname_,
			PostingIteratorInterface* structitr_,
			const std::vector<PostingIteratorInterface*>& itrs_,
			MetaDataReaderInterface* metadata_,
			const std::vector<ArithmeticVariant>&) const
	{
		if (itrs_.size() || structitr_) throw std::runtime_error( "no feature sets as arguments expected for summarizer 'metadata'");
		return new SummarizerClosureMetaData( metadata_, elementname_);
	}
};

}//namespace
#endif


