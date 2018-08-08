/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface to create a summarizer from a weighting function
#include "weightingFunctionSummarizer.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/reference.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
using namespace strus;

/// \brief Summarizer context created from a weighting function context
class WeightingFunctionSummarizerContext
	:public SummarizerFunctionContextInterface
{
public:
	WeightingFunctionSummarizerContext( const char* name_, ErrorBufferInterface* errorhnd_, WeightingFunctionContextInterface* func_, const std::string& resultname_)
		:m_errorhnd(errorhnd_),m_name(name_),m_func( func_),m_resultname(resultname_){}

	virtual ~WeightingFunctionSummarizerContext()
	{
		delete m_func;
	}

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* postingIterator_,
			const std::vector<SummarizationVariable>&,
			double weight_,
			const TermStatistics& stats_)
	{
		m_func->addWeightingFeature( name_, postingIterator_, weight_, stats_);
	}

	virtual void setVariableValue( const std::string& name, double value)
	{
		m_func->setVariableValue( name, value);
	}

	virtual std::vector<SummaryElement> getSummary( const Index& docno)
	{
		try
		{
			std::ostringstream out;
			out << m_func->call( docno);
			std::vector<SummaryElement> rt;
			if (m_resultname.empty())
			{
				rt.push_back( SummaryElement( m_name, out.str()));
			}
			else
			{
				rt.push_back( SummaryElement( m_resultname, out.str()));
			}
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in debug call of '%s' summarizer context: %s"), m_name, *m_errorhnd, std::vector<SummaryElement>());
	}

	virtual std::string debugCall( const Index& docno)
	{
		try
		{
			return m_func->debugCall( docno);
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in debug call of '%s' summarizer context: %s"), m_name, *m_errorhnd, std::string());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const char* m_name;
	WeightingFunctionContextInterface* m_func;
	std::string m_resultname;
};


/// \brief Summarizer instance created from a weighting function instance
class WeightingFunctionSummarizerInstance
	:public SummarizerFunctionInstanceInterface
{
public:
	WeightingFunctionSummarizerInstance( const char* name_, ErrorBufferInterface* errorhnd_, WeightingFunctionInstanceInterface* func_)
		:m_errorhnd(errorhnd_),m_name(name_),m_func( func_),m_resultname(){}

	virtual ~WeightingFunctionSummarizerInstance()
	{
		delete m_func;
	}

	virtual void addStringParameter(
			const std::string& name,
			const std::string& value)
	{
		m_func->addStringParameter( name, value);
	}

	virtual void addNumericParameter(
			const std::string& name,
			const NumericVariant& value)
	{
		m_func->addNumericParameter( name, value);
	}

	virtual void defineResultName(
			const std::string& resultname,
			const std::string& itemname)
	{
		try
		{
			if (strus::caseInsensitiveEquals( itemname, "weight")
			||  strus::caseInsensitiveEquals( itemname, m_name))
			{
				m_resultname = resultname;
			}
			else
			{
				throw strus::runtime_error( _TXT("unknown item name '%s"), itemname.c_str());
			}
		}
		CATCH_ERROR_ARG1_MAP( _TXT("error defining result name of '%s' summarizer: %s"), m_name, *m_errorhnd);
	}

	virtual std::vector<std::string> getVariables() const
	{
		return m_func->getVariables();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata_,
			const GlobalStatistics& stats) const
	{
		try
		{
			Reference<WeightingFunctionContextInterface> context( m_func->createFunctionContext( storage_, metadata_, stats));
			if (!context.get()) throw strus::runtime_error(_TXT("error creating '%s' weighting function context"), m_name);
			Reference<SummarizerFunctionContextInterface> rt( new WeightingFunctionSummarizerContext( m_name, m_errorhnd, context.get(), m_resultname));
			context.release();
			return rt.release();
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' summarizer context: %s"), m_name, *m_errorhnd, 0);
	}

	virtual std::string tostring() const
	{
		try
		{
			std::ostringstream out;
			out << "summarizer weighting '" << m_name << "':" << std::endl;
			out << m_func->tostring();
			return out.str();
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), m_name, *m_errorhnd, std::string());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	const char* m_name;
	WeightingFunctionInstanceInterface* m_func;
	std::string m_resultname;
};


/// \brief Summarizer created from a weighting function
class WeightingFunctionSummarizer
	:public SummarizerFunctionInterface
{
public:
	WeightingFunctionSummarizer( const std::string& name_, ErrorBufferInterface* errorhnd_, const WeightingFunctionInterface* func_)
		:m_errorhnd(errorhnd_),m_name(name_),m_func( func_){}

	virtual ~WeightingFunctionSummarizer()
	{}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const
	{
		try
		{
			Reference<WeightingFunctionInstanceInterface> funcinstance( m_func->createInstance( processor));
			if (!funcinstance.get()) throw strus::runtime_error(_TXT("error creating '%s' weighting function instance: %s"), m_name.c_str(), m_errorhnd->fetchError());
			Reference<SummarizerFunctionInstanceInterface> rt( new WeightingFunctionSummarizerInstance( m_name.c_str(), m_errorhnd, funcinstance.get()));
			funcinstance.release();
			return rt.release();
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' summarizer instance: %s"), m_name.c_str(), *m_errorhnd, 0);
	}

	virtual FunctionDescription getDescription() const
	{
		try
		{
			return FunctionDescription( m_func->getDescription(), string_format( _TXT("summarizer derived from weighting function %s: "), m_name.c_str()));
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' summarizer description: %s"), m_name.c_str(), *m_errorhnd, FunctionDescription());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	std::string m_name;
	const WeightingFunctionInterface* m_func;
};


SummarizerFunctionInterface* strus::createSummarizerFromWeightingFunction( const std::string& name_, ErrorBufferInterface* errorhnd_, const WeightingFunctionInterface* func_)
{
	try
	{
		return new WeightingFunctionSummarizer( name_, errorhnd_, func_);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' summarizer from weighting function: %s"), name_.c_str(), *errorhnd_, 0);
}



