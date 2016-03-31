/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a summarizer function type
/// \file summarizerFunctionInterface.hpp
#ifndef _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#include "strus/functionDescription.hpp"
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class SummarizerFunctionInstanceInterface;
/// \brief Forward declaration
class QueryProcessorInterface;


/// \brief Interface for summarization functions (additional info about the matches in the result ranklist of a retrieval query)
class SummarizerFunctionInterface
{
public:
	/// \brief Destructor
	virtual ~SummarizerFunctionInterface(){}

	/// \brief Create an instance of this summarization function for parametrization
	/// \param[in] processor provider for query processing functions
	/// \return the created summarization function instance (ownership to caller)
	virtual SummarizerFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const=0;

	/// \brief Get a description of the function for user help and introspection
	/// \return the description structure
	virtual FunctionDescription getDescription() const=0;
};

}//namespace
#endif


