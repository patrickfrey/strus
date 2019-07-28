/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a scalar function parser
/// \file scalarFunctionParserInterface.hpp
#ifndef _STRUS_SCALAR_FUNCTION_PARSER_INTERFACE_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_PARSER_INTERFACE_HPP_INCLUDED
#include "strus/structView.hpp"
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class ScalarFunctionInterface;

/// \brief Interface for parsing scalar function definitions
class ScalarFunctionParserInterface
{
public:
	/// \brief Destructor
	virtual ~ScalarFunctionParserInterface(){}

	/// \brief Parses a function from source and instantiates a function object
	/// \param[in] src source describing the function
	/// \param[in] argumentNames the list of function argument names in the source (default is a prefix '_' followed by the argument index starting with 0)
	/// \return the created function object
	virtual ScalarFunctionInterface* createFunction(
			const std::string& src,
			const std::vector<std::string>& argumentNames) const=0;

	/// \brief Return a structure with all definitions for introspection
	/// \return the structure with all definitions for introspection
	virtual StructView view() const=0;
};

}// namespace
#endif

