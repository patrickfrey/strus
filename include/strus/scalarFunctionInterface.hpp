/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a scalar function type
/// \file scalarFunctionInterface.hpp
#ifndef _STRUS_SCALAR_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class ScalarFunctionInstanceInterface;

/// \brief Interface for a scalar function type
/// \note A scalar function is an N-ary function with a fixed number of arguments to a single value
///	(http://mathworld.wolfram.com/ScalarFunction.html). For strus we reduce the value domain to
///	double precision floating point values.
class ScalarFunctionInterface
{
public:
	/// \brief Destructor
	virtual ~ScalarFunctionInterface(){}

	/// \brief Get the list of variables the function is parameterized with
	/// \return list of variable names
	virtual std::vector<std::string> getVariables() const=0;

	/// \brief Get the number of arguments of this function
	/// \return the number of arguments
	virtual unsigned int getNofArguments() const=0;

	/// \brief Set a variable value default
	/// \param[in] name variable name
	/// \param[in] value default value of the variable to set
	/// \remark Reports an error, if the domain does not match
	virtual void setDefaultVariableValue( const std::string& name, double value)=0;

	/// \brief Creates an instance of the function
	/// \return the created scalar function instance
	virtual ScalarFunctionInstanceInterface* createInstance() const=0;

	/// \brief Return the representation (VM code or whatever it is) of the function as string
	virtual std::string tostring() const=0;
};

}// namespace
#endif

