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
#include "private/functionDescription.hpp"
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
	WeightingFunctionSummarizerContext( const std::string& name_, ErrorBufferInterface* errorhnd_, WeightingFunctionContextInterface* func_)
		:m_errorhnd(errorhnd_),m_name(name_),m_func( func_){}

	virtual ~WeightingFunctionSummarizerContext()
	{
		delete m_func;
	}

	virtual void addSummarizationFeature(
			const std::string& name_,
			PostingIteratorInterface* postingIterator_,
			const std::vector<SummarizationVariable>&,
			double weight_)
	{
		m_func->addWeightingFeature( name_, postingIterator_, weight_);
	}

	virtual void setVariableValue( const std::string& name_, double value)
	{
		m_func->setVariableValue( name_, value);
	}

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc)
	{
		try
		{
			std::ostringstream out;
			const std::vector<WeightedField>& wf = m_func->call( doc.docno());
			std::vector<WeightedField>::const_iterator wi = wf.begin(), we = wf.end();
			std::vector<SummaryElement> rt;
			for (int widx=0; wi != we; ++wi,++widx)
			{
				if (wi->field() == doc.field())
				{
					if (wi->field().defined())
					{
						rt.push_back( SummaryElement( "start", strus::string_format("%d", (int)wi->field().start()), widx));
						rt.push_back( SummaryElement( "end", strus::string_format("%d", (int)wi->field().end()), widx));
					}
					rt.push_back( SummaryElement( "weight", strus::string_format("%.4f", wi->weight()), widx));
				}
			}
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in call of '%s' summarizer function: %s"), m_name.c_str(), *m_errorhnd, std::vector<SummaryElement>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	std::string m_name;
	WeightingFunctionContextInterface* m_func;
};


/// \brief Summarizer instance created from a weighting function instance
class WeightingFunctionSummarizerInstance
	:public SummarizerFunctionInstanceInterface
{
public:
	WeightingFunctionSummarizerInstance( const std::string& name_, ErrorBufferInterface* errorhnd_, WeightingFunctionInstanceInterface* func_)
		:m_errorhnd(errorhnd_),m_name(name_),m_func( func_){}

	virtual ~WeightingFunctionSummarizerInstance()
	{
		delete m_func;
	}

	virtual void addStringParameter(
			const std::string& name_,
			const std::string& value)
	{
		m_func->addStringParameter( name_, value);
	}

	virtual void addNumericParameter(
			const std::string& name_,
			const NumericVariant& value)
	{
		m_func->addNumericParameter( name_, value);
	}

	virtual std::vector<std::string> getVariables() const
	{
		return m_func->getVariables();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics& stats) const
	{
		try
		{
			Reference<WeightingFunctionContextInterface> context( m_func->createFunctionContext( storage_, stats));
			if (!context.get()) throw strus::runtime_error(_TXT("error creating '%s' weighting function context"), m_name.c_str());
			Reference<SummarizerFunctionContextInterface> rt( new WeightingFunctionSummarizerContext( m_name, m_errorhnd, context.get()));
			context.release();
			return rt.release();
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' summarizer context: %s"), m_name.c_str(), *m_errorhnd, 0);
	}

	virtual bool doPopulate() const
	{
		return false;
	}

	virtual const char* name() const	{return m_name.c_str();}

	virtual StructView view() const	
	{
		try
		{
			StructView rt;
			rt( "name", m_name);
			rt( "weighting", m_func->view());
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error mapping '%s' summarizer to string: %s"), m_name.c_str(), *m_errorhnd, std::string());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	std::string m_name;
	WeightingFunctionInstanceInterface* m_func;
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

	virtual const char* name() const	{return m_name.c_str();}

	virtual StructView view() const
	{
		try
		{
			return FunctionDescription( m_func->view(), string_format( _TXT("summarizer derived from weighting function %s: "), m_name.c_str()));
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



