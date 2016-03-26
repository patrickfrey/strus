/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for parameterizing a scalar function
/// \file scalarFunctionInstanceInterface.hpp
#ifndef _STRUS_SCALAR_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>

namespace strus
{
/// \brief Forward declaration
class ScalarFunctionContextInterface;

/// \brief Interface for parameterizing a scalar function
class ScalarFunctionInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~ScalarFunctionInstanceInterface(){}

	/// \brief Get the list of variables the function is parameterized with
	/// \return list of variable names
	virtual std::vector<std::string> getVariables() const=0;

	/// \brief Get the number of arguments of this function
	/// \return the number of arguments
	virtual std::size_t getNofArguments() const=0;

	/// \brief Initialize a variable value
	/// \param[in] name variable name
	/// \param[in] value value of the variable to check
	/// \remark Reports an error, if the domain does not match
	virtual void setVariableValue( const std::string& name, double value)=0;

	/// \brief Execute the function
	/// \param[in] args array of arguments
	/// \param[in] nofargs number of elements in args
	virtual double call( const double* args, std::size_t nofargs) const=0;

	/// \brief Return the representation (VM code or whatever it is) of the function with variables substituted as string
	virtual std::string tostring() const=0;
};

} //namespace
#endif


