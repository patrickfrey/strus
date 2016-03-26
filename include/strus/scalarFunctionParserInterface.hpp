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

	typedef double (*BinaryFunction)( double arg1, double arg2);
	typedef double (*UnaryFunction)( double arg);
	typedef double (*NaryFunction)( std::size_t nofargs, const double* args);

	/// \brief Define a binary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	virtual void defineBinaryFunction( const std::string& name, BinaryFunction func)=0;

	/// \brief Define an unary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	virtual void defineUnaryFunction( const std::string& name, UnaryFunction func)=0;

	/// \brief Define an N-ary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	virtual void defineNaryFunction( const std::string& name, NaryFunction func)=0;

	/// \brief Parses a function from source and instantiates a function object
	/// \param[in] src source describing the function
	/// \return the created function object
	virtual ScalarFunctionInterface* createFunction( const std::string& src) const=0;
};

}// namespace
#endif

