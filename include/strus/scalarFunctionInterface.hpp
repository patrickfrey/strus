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

	/// \brief Creates an instance of the function
	/// \return the created scalar function instance
	virtual ScalarFunctionInstanceInterface* createInstance() const=0;

	/// \brief Return the representation (VM code or whatever it is) of the function as string
	virtual std::string tostring() const=0;
};

}// namespace
#endif

