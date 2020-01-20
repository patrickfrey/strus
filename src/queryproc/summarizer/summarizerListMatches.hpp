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
	SummarizerFunctionContextListMatches( unsigned int maxNofMatches_, ErrorBufferInterface* errorhnd_)
		:m_maxNofMatches(maxNofMatches_),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionContextListMatches(){}

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double /*weight*/,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

	virtual std::string debugCall( const strus::WeightedDocument& doc);

private:
	const StorageClientInterface* m_storage;			///< storage interface
	std::vector<PostingIteratorInterface*> m_itrs;			///< iterators for summarization
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
		:m_maxNofMatches(100),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionInstanceListMatches(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface*,
			const GlobalStatistics&) const;

	virtual const char* name() const {return "matchpos";}
	virtual StructView view() const;

private:
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

	virtual const char* name() const {return "matchpos";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


