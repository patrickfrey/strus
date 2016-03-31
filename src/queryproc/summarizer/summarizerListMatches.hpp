/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	SummarizerFunctionContextListMatches( const std::string& resultname_, unsigned int maxNofMatches_, ErrorBufferInterface* errorhnd_)
		:m_resultname(resultname_),m_maxNofMatches(maxNofMatches_),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionContextListMatches(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			float /*weight*/,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	const StorageClientInterface* m_storage;			///< storage interface
	std::vector<PostingIteratorInterface*> m_itrs;			///< iterators for summarization
	std::string m_resultname;					///< result element name
	unsigned int m_maxNofMatches;					///< maximum number of matches to list
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceListMatches
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceListMatches
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceListMatches( ErrorBufferInterface* errorhnd_)
		:m_resultname("position"),m_maxNofMatches(100),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionInstanceListMatches(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_resultname;					///< result element name
	unsigned int m_maxNofMatches;					///< maximum number of matches to list
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

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


