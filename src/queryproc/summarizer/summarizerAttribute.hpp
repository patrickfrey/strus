/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_SUMMARIZER_ATTRIBUTE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_ATTRIBUTE_HPP_INCLUDED
#include "strus/summarizerFunctionContextInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace strus
{

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class SummarizerFunctionContextAttribute
	:public SummarizerFunctionContextInterface
{
public:
	/// \param[in] attribreader_ reader for document attributes
	/// \param[in] name_ attribute identifier
	SummarizerFunctionContextAttribute( AttributeReaderInterface* attribreader_, const std::string& name_, ErrorBufferInterface* errorhnd_);

	virtual ~SummarizerFunctionContextAttribute();

	virtual void addSummarizationFeature(
			const std::string&,
			PostingIteratorInterface*,
			const std::vector<SummarizationVariable>&,
			float /*weight*/,
			const TermStatistics&);

	virtual std::vector<SummaryElement> getSummary( const Index& docno);

private:
	AttributeReaderInterface* m_attribreader;			///< attribute reader interface
	std::string m_attribname;					///< attribute name to output
	int m_attrib;							///< attribute handle to output
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class SummarizerFunctionInstanceAttribute
/// \brief Summarizer instance for retrieving meta data
class SummarizerFunctionInstanceAttribute
	:public SummarizerFunctionInstanceInterface
{
public:
	explicit SummarizerFunctionInstanceAttribute( ErrorBufferInterface* errorhnd_)
		:m_name(),m_errorhnd(errorhnd_){}

	virtual ~SummarizerFunctionInstanceAttribute(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage,
			MetaDataReaderInterface*,
			const GlobalStatistics&) const;

	virtual std::string tostring() const;

private:
	std::string m_name;						///< attribute name to output
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class SummarizerFunctionAttribute
	:public SummarizerFunctionInterface
{
public:
	explicit SummarizerFunctionAttribute( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	SummarizerFunctionAttribute(){}
	virtual ~SummarizerFunctionAttribute(){}

	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface*) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


