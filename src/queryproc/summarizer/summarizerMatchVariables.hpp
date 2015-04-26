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
#include "strus/summarizerExecutionContextInterface.hpp"
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


class SummarizerExecutionContextMatchVariables
	:public SummarizerExecutionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] type_ type of the forward index tokens to build the summary with
	/// \param[in] delimiter_ delimiter to print between multiple output elements
	/// \param[in] assing_ assingment operator to use for output
	SummarizerExecutionContextMatchVariables(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const std::string& type_,
			const std::string& delimiter_,
			const std::string& assign_);

	virtual ~SummarizerExecutionContextMatchVariables(){}

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
};


class SummarizerFunctionInstanceMatchVariables
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceMatchVariables()
		:m_type(){}

	virtual ~SummarizerFunctionInstanceMatchVariables(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual SummarizerExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage,
			const QueryProcessorInterface* processor,
			MetaDataReaderInterface*) const
	{
		if (m_type.empty())
		{
			throw strus::runtime_error( _TXT( "emtpy forward index type definition (parameter 'type') in match phrase summarizer configuration"));
		}
		return new SummarizerExecutionContextMatchVariables( storage, processor, m_type, m_delimiter, m_assign);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << "type='" << m_type 
			<< "', delimiter='" << m_delimiter
			<< "', assign='" << m_assign << "'";
		return rt.str();
	}

private:
	std::string m_type;
	std::string m_delimiter;
	std::string m_assign;
};


class SummarizerFunctionMatchVariables
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionMatchVariables(){}
	virtual ~SummarizerFunctionMatchVariables(){}

	virtual SummarizerFunctionInstanceInterface* createInstance() const
	{
		return new SummarizerFunctionInstanceMatchVariables();
	}
};

}//namespace
#endif


