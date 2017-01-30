/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_ACCUMULATE_VARIABLE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_ACCUMULATE_VARIABLE_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

struct AccumulateVariableData
{
	std::string type;		//< forward index type
	std::string var;		//< variable to accumulate
	std::string resultname;		//< name of results (default variable name)
	double norm;			//< normalization factor for end result weights
	double pairmul;			//< multiplicator for matching pairs
	unsigned int maxNofElements;	//< maximum number of best elements to return

	AccumulateVariableData()
		:type(),var(),resultname(),norm(1.0),pairmul(1.0),maxNofElements(30){}
	AccumulateVariableData( const AccumulateVariableData& o)
		:type(o.type),var(o.var),resultname(o.resultname),norm(o.norm),pairmul(o.pairmul),maxNofElements(o.maxNofElements){}
};

class SummarizerFunctionContextAccumulateVariable
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] data_ parameter
	/// \param[in] errorhnd_ error buffer
	SummarizerFunctionContextAccumulateVariable(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const Reference<AccumulateVariableData> data_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextAccumulateVariable(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	struct SummarizationFeature
	{
		PostingIteratorInterface* itr;
		std::vector<const PostingIteratorInterface*> varitr;
		double weight;

		SummarizationFeature( PostingIteratorInterface* itr_, const std::vector<const PostingIteratorInterface*>& varitr_, double weight_)
			:itr(itr_),varitr(varitr_),weight(weight_){}
		SummarizationFeature( const SummarizationFeature& o)
			:itr(o.itr),varitr(o.varitr),weight(o.weight){}
	};

private:
	const StorageClientInterface* m_storage;			///< storage interface
	const QueryProcessorInterface* m_processor;			///< query processor
	Reference<ForwardIteratorInterface> m_forwardindex;		///< forward index interface
	Reference<AccumulateVariableData> m_data;			///< parameters
	std::vector<SummarizationFeature> m_features;			///< features used for summarization
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionInstanceAccumulateVariable
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceAccumulateVariable( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_data(new AccumulateVariableData()),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAccumulateVariable(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);
	virtual void defineResultName( const std::string& resultname, const std::string& itemname);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	Reference<AccumulateVariableData> m_data;	///< parameters
	const QueryProcessorInterface* m_processor;	///< query processor
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


class SummarizerFunctionAccumulateVariable
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionAccumulateVariable( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionAccumulateVariable(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


