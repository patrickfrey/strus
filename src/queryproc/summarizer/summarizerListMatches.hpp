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
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
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
/// \brief Forward declaration
class ErrorBufferInterface;

class SummarizerFunctionContextListMatches
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextListMatches( unsigned int maxNofMatches_, ErrorBufferInterface* errorhnd_)
		:m_maxNofMatches(maxNofMatches_),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionContextListMatches(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			float /*weight*/);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;
	std::vector<PostingIteratorInterface*> m_itrs;
	unsigned int m_maxNofMatches;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceListMatches
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceListMatches
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceListMatches( ErrorBufferInterface* errorhnd_)
		:m_maxNofMatches(100),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionInstanceListMatches(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface*) const;

	virtual std::string tostring() const;

private:
	unsigned int m_maxNofMatches;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionListMatches
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionListMatches( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	SummarizerFunctionListMatches(){}

	virtual ~SummarizerFunctionListMatches(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual const char* getDescription() const
	{
		return _TXT("Get the list of occurencies printed for the features specified with the feature parameter 'match'. With the parameter 'N' you can specify the maximum number of matches to return");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


