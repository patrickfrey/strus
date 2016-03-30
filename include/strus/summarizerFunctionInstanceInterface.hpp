/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a parameterized summarizer function instance
/// \file summarizerFunctionInstanceInterface.hpp
#ifndef _STRUS_SUMMARIZER_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/globalStatistics.hpp"
#include <string>

namespace strus
{

/// \brief Forward declaration
class NumericVariant;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class SummarizerFunctionContextInterface;


/// \brief Interface for a parameterized instance of summarization
class SummarizerFunctionInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~SummarizerFunctionInstanceInterface(){}

	/// \brief Add a named string value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addStringParameter(
			const std::string& name,
			const std::string& value)=0;

	/// \brief Add a named numeric value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addNumericParameter(
			const std::string& name,
			const NumericVariant& value)=0;

	/// \brief Create an execution context for this summarization function instance
	/// \param[in] storage_ storage interface for getting information for summarization (like for example document attributes)
	/// \param[in] metadata_ metadata interface for inspecting document meta data (like for example the document insertion date)
	/// \param[in] stats global statistics for weighting
	/// \return the execution context, the summarization function instance with its execution context (ownership to caller)
	virtual SummarizerFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata_,
			const GlobalStatistics& stats) const=0;

	/// \brief Get a comma ',' separated list of the function parameters as assignments (e.g. name=value)
	/// \return the parameter list as string
	virtual std::string tostring() const=0;
};

}//namespace
#endif


