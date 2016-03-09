/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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
			float weight,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	struct SummarizationFeature
	{
		PostingIteratorInterface* itr;
		std::vector<const PostingIteratorInterface*> varitr;
		float weight;

		SummarizationFeature( PostingIteratorInterface* itr_, const std::vector<const PostingIteratorInterface*>& varitr_, float weight_)
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
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

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

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


