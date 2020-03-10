/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_CONTENT_HPP_INCLUDED
#define _STRUS_SUMMARIZER_CONTENT_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/reference.hpp"
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

class SummarizerFunctionContextContent
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextContent( 
			const StorageClientInterface* storage_,
			const std::string& type_, unsigned int maxNofMatches_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextContent(){}

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double /*weight*/);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

private:
	const StorageClientInterface* m_storage;			///< storage interface
	Reference<ForwardIteratorInterface> m_forwardindex;		///< forward index iterator
	std::string m_type;						///< forward index type name
	unsigned int m_maxNofMatches;					///< maximum number of matches to return
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceContent
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceContent
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceContent( ErrorBufferInterface* errorhnd_)
		:m_type(),m_maxNofMatches(100),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionInstanceContent(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics&) const;

	virtual bool doPopulate() const
	{
		return false;
	}
	virtual const char* name() const;
	virtual StructView view() const;

private:
	std::string m_type;						///< forward index type name to fetch
	unsigned int m_maxNofMatches;					///< maximum number of matches to return
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionContent
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionContent( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	SummarizerFunctionContent(){}

	virtual ~SummarizerFunctionContent(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


