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
	/// \return the created function object
	virtual ScalarFunctionInterface* createFunction( const std::string& src) const=0;
};

}// namespace
#endif

