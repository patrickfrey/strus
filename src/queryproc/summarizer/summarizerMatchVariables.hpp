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
#ifndef _STRUS_SUMMARIZER_MATCH_VARIABLES_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCH_VARIABLES_HPP_INCLUDED
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


class SummarizerFunctionContextMatchVariables
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the forward index tokens to build the summary with
	/// \param[in] delimiter_ delimiter to print between multiple output elements
	/// \param[in] assing_ assingment operator to use for output
	SummarizerFunctionContextMatchVariables(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			const std::string& delimiter_,
			const std::string& assign_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextMatchVariables(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	struct SummarizationFeature
	{
		PostingIteratorInterface* itr;
		std::vector<SummarizationVariable> variables;

		SummarizationFeature( PostingIteratorInterface* itr_, const std::vector<SummarizationVariable>& variables_)
			:itr(itr_),variables(variables_){}
		SummarizationFeature( const SummarizationFeature& o)
			:itr(o.itr),variables(o.variables){}
	};

private:
	const StorageClientInterface* m_storage;
	const QueryProcessorInterface* m_processor;
	Reference<ForwardIteratorInterface> m_forwardindex;
	std::string m_type;
	std::string m_delimiter;
	std::string m_assign;
	std::vector<SummarizationFeature> m_features;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionInstanceMatchVariables
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchVariables( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_type(),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchVariables(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*) const;

	virtual std::string tostring() const;

private:
	std::string m_type;
	std::string m_delimiter;
	std::string m_assign;
	const QueryProcessorInterface* m_processor;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionMatchVariables
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionMatchVariables( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionMatchVariables(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* getDescription() const
	{
		return _TXT("Extract all variables assigned to subexpressions of features specified with the feature parameter 'match'. The feature type of the values extracted from the forward index as variable values are specified with the string parameter 'type'. With 'assign' you can specify the assignment operator used in the result other than '=' (default). With 'delimiter' you can specify the separator between two results other than ',' (default).");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


