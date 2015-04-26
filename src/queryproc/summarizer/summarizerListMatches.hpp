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
#ifndef _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#define _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerExecutionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;

class SummarizerExecutionContextListMatches
	:public SummarizerExecutionContextInterface
{
public:
	SummarizerExecutionContextListMatches(){}
	virtual ~SummarizerExecutionContextListMatches(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;
	std::vector<PostingIteratorInterface*> m_itrs;
};


/// \class SummarizerFunctionInstanceListMatches
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceListMatches
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceListMatches(){}
	virtual ~SummarizerFunctionInstanceListMatches(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerExecutionContextInterface* createExecutionContext(
			const StorageClientInterface*,
			const QueryProcessorInterface*,
			MetaDataReaderInterface*) const
	{
		return new SummarizerExecutionContextListMatches();
	}

	virtual std::string tostring() const
	{
		return std::string();
	}
};


class SummarizerFunctionListMatches
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionListMatches(){}

	virtual ~SummarizerFunctionListMatches(){}

	virtual SummarizerFunctionInstanceInterface* createInstance() const
	{
		return new SummarizerFunctionInstanceListMatches();
	}
};


}//namespace
#endif


