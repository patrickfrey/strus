/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include <map>
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

struct MatchVariablesData
{
	typedef std::map<std::string,std::string> NameMap;
	std::string type;		//< forward index type
	NameMap namemap;		//< map for renaming results (defaultvariable names)

	MatchVariablesData()
		:type(),namemap(){}
	MatchVariablesData( const MatchVariablesData& o)
		:type(o.type),namemap(o.namemap){}
};

class SummarizerFunctionContextMatchVariables
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] processor_ query processor to use
	/// \param[in] data_ parameter data for evaluation
	SummarizerFunctionContextMatchVariables(
			const StorageClientInterface* storage_,
			const QueryProcessorInterface* processor_,
			const Reference<MatchVariablesData>& data_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextMatchVariables(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

	virtual std::string debugCall( const Index& docno);

private:
	struct SummarizationFeature
	{
		PostingIteratorInterface* itr;
		std::vector<SummarizationVariable> variables;
		double weight;

		SummarizationFeature( PostingIteratorInterface* itr_, const std::vector<SummarizationVariable>& variables_, double weight_)
			:itr(itr_),variables(variables_),weight(weight_){}
		SummarizationFeature( const SummarizationFeature& o)
			:itr(o.itr),variables(o.variables),weight(o.weight){}
	};

private:
	const StorageClientInterface* m_storage;		///< storage to access
	const QueryProcessorInterface* m_processor;		///< query processor interface for object creation
	Reference<ForwardIteratorInterface>m_forwardindex;	///< forward index iterators for extracting features
	Reference<MatchVariablesData> m_data;			///< parameters
	std::vector<SummarizationFeature> m_features;		///< features to weight
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class SummarizerFunctionInstanceMatchVariables
	:public SummarizerFunctionInstanceInterface
{
public:
	SummarizerFunctionInstanceMatchVariables( const QueryProcessorInterface* processor_, ErrorBufferInterface* errorhnd_)
		:m_data(new MatchVariablesData()),m_processor(processor_),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMatchVariables(){}

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
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	Reference<MatchVariablesData> m_data;
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

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


