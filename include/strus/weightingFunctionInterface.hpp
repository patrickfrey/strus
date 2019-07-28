/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a weighting function type
/// \file weightingFunctionInterface.hpp
#ifndef _STRUS_WEIGHTING_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_FUNCTION_INTERFACE_HPP_INCLUDED
#include "strus/structView.hpp"
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class WeightingFunctionInstanceInterface;
/// \brief Forward declaration
class QueryProcessorInterface;

/// \brief Interface for a weighting function that can be used for ranking in the query evaluation
class WeightingFunctionInterface
{
public:
	/// \brief Destructor
	virtual ~WeightingFunctionInterface(){}

	/// \brief Create an instance of this function for parametrization
	/// \param[in] processor provider for query processing functions
	/// \return the created function instance (ownership to caller)
	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const=0;

	/// \brief Get the name of the function
	/// \return the identifier
	virtual const char* name() const=0;

	/// \brief Return a structure with all definitions for introspection
	/// \return the structure with all definitions for introspection
	virtual StructView view() const=0;
};

}//namespace
#endif

