/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_FORWARD_INDEX_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FORWARD_INDEX_HPP_INCLUDED
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

class SummarizerFunctionContextForwardIndex
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextForwardIndex( 
			const StorageClientInterface* storage_,
			const std::string& resultname_, const std::string& type_, unsigned int maxNofMatches_,
			ErrorBufferInterface* errorhnd_);
	virtual ~SummarizerFunctionContextForwardIndex(){}

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>&,
			double /*weight*/,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	const StorageClientInterface* m_storage;			///< storage interface
	Reference<ForwardIteratorInterface> m_forwardindex;		///< forward index iterator
	std::string m_resultname;					///< result element name
	std::string m_type;						///< result element name
	unsigned int m_maxNofMatches;					///< maximum number of matches to return
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceForwardIndex
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceForwardIndex
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceForwardIndex( ErrorBufferInterface* errorhnd_)
		:m_resultname(),m_type(),m_maxNofMatches(100),m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionInstanceForwardIndex(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual void defineResultName(
			const std::string& resultname,
			const std::string& itemname);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics&) const;

	virtual const char* name() const {return "forwardindex";}
	virtual StructView view() const;

private:
	std::string m_resultname;					///< result element name
	std::string m_type;						///< forward index type name to fetch
	unsigned int m_maxNofMatches;					///< maximum number of matches to return
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionForwardIndex
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionForwardIndex( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	SummarizerFunctionForwardIndex(){}

	virtual ~SummarizerFunctionForwardIndex(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual const char* name() const {return "forwardindex";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


