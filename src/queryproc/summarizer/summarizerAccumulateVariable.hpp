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


class SummarizerFunctionContextAccumulateVariable
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the forward index tokens to build the summary with
	/// \param[in] var_ variable to extract
	/// \param[in] norm_ weight normalization factor
	/// \param[in] maxNofElements_ number of best matches to inspect
	/// \param[in] errorhnd_ error buffer
	SummarizerFunctionContextAccumulateVariable(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			const std::string& var_,
			double norm_,
			unsigned int maxNofElements_,
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
	std::string m_type;						///< forward index type for extraction of result elements
	std::string m_var;						///< name of variable to accumulate
	double m_norm;							///< normalization factor for end result weights
	unsigned int m_maxNofElements;					///< maximum number of best elements to return
	std::vector<SummarizationFeature> m_features;			///< features used for summarization
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionInstanceAccumulateVariable
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceAccumulateVariable( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_type(),m_var(),m_norm(1.0),m_maxNofElements(30),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAccumulateVariable(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_type;				///< forward index type to extract
	std::string m_var;				///< variable name to extract for accumulation
	double m_norm;					///< normalization factor
	unsigned int m_maxNofElements;			///< number of best matches to inspect
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


