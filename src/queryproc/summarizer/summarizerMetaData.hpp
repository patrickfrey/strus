/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_METADATA_HPP_INCLUDED
#define _STRUS_SUMMARIZER_METADATA_HPP_INCLUDED
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Interface for the summarization context (of a SummarizationFunction)
class SummarizerFunctionContextMetaData
	:public SummarizerFunctionContextInterface
{
public:
	/// \brief Constructor
	/// \param[in] metadata_ reader for document meta data
	/// \param[in] metaname_ meta data field identifier
	SummarizerFunctionContextMetaData( MetaDataReaderInterface* metadata_, const std::string& metaname_, ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextMetaData(){}

	virtual void addSummarizationFeature(
			const std::string&,
			PostingIteratorInterface*,
			const std::vector<SummarizationVariable>&,
			double /*weight*/);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual std::vector<SummaryElement> getSummary( const strus::WeightedDocument& doc);

private:
	strus::Reference<MetaDataReaderInterface> m_metadata;
	std::string m_metaname;
	int m_attrib;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceMetaData
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceMetaData
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceMetaData( ErrorBufferInterface* errorhnd_)
		:m_metaname(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceMetaData(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);

	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

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
	std::string m_metaname;
	ErrorBufferInterface* m_errorhnd;	///< buffer for error messages
};


class SummarizerFunctionMetaData
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionMetaData( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionMetaData(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual const char* name() const;
	virtual StructView view() const;
	
private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


