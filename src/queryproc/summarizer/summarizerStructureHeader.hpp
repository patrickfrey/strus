/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_STRUCTURE_HEADER_HPP_INCLUDED
#define _STRUS_SUMMARIZER_STRUCTURE_HEADER_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/storage/summarizationVariable.hpp"
#include "strus/reference.hpp"
#include "forwardIndexTextCollector.hpp"
#include "structureSearch.hpp"
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
class QueryProcessorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

struct SummarizerFunctionParameterStructureHeader
{
	std::string textType;					///< name of the forward index feature to collect as text
	std::string structName;					///< name of structure to inspect for headers or empty if to scan all structures

	SummarizerFunctionParameterStructureHeader()
		:textType()
		,structName()
	{}
	SummarizerFunctionParameterStructureHeader( const SummarizerFunctionParameterStructureHeader& o)
		:textType(o.textType)
		,structName(o.structName)
	{}
};

class SummarizerFunctionContextStructureHeader
	:public SummarizerFunctionContextInterface
{
public:
	SummarizerFunctionContextStructureHeader(
			const StorageClientInterface* storage_,
			const SummarizerFunctionParameterStructureHeader& parameter_,
			ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextStructureHeader(){}

	virtual void addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&);

	virtual void setVariableValue( const std::string& name, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

private:
	SummarizerFunctionParameterStructureHeader m_parameter;		///< parameter
	const StorageClientInterface* m_storage;			///< storage client interface
	strus::Reference<StructureIteratorInterface> m_structiter;		///< structure iterator
	strus::Index m_structno;					///< type of structure to scan for results or 0 if to scan all
	std::vector<StructureHeaderField> m_headerar;			///< temporary result, header fields from which summary is constructed 
	ForwardIndexTextCollector m_textCollector;			///< structure for collecting the summary texts
	ErrorBufferInterface* m_errorhnd;				///< buffer for error reporting
};


class SummarizerFunctionInstanceStructureHeader
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceStructureHeader( ErrorBufferInterface* errorhnd_)
		:m_parameter(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceStructureHeader(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const;

	virtual bool doPopulate() const
	{
		return false;
	}

	virtual const char* name() const;
	virtual StructView view() const;

private:
	SummarizerFunctionParameterStructureHeader m_parameter;	///< summarizer function parameters
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class SummarizerFunctionStructureHeader
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionStructureHeader( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~SummarizerFunctionStructureHeader(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const;
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


