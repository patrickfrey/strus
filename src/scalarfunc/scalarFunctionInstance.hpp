/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Impementation for parameterizing a scalar function built with the standard scalar function parser of strus
/// \file scalarFunctionInstance.hpp
#ifndef _STRUS_SCALAR_FUNCTION_INSTANCE_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_INSTANCE_IMPLEMENTATION_HPP_INCLUDED
#include "strus/scalarFunctionInstanceInterface.hpp"
#include "scalarFunction.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface for parameterizing a scalar function
class ScalarFunctionInstance
	:public ScalarFunctionInstanceInterface
{
public:
	ScalarFunctionInstance( const ScalarFunction* func_, const std::vector<double>& valuear_, std::size_t nof_args_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_func(func_),m_valuear(valuear_),m_nof_args(nof_args_){}

	virtual ~ScalarFunctionInstance(){}

	virtual void setVariableValue( const std::string& name, double value);

	virtual double call( const double* args, std::size_t nofargs) const;

	virtual std::string tostring() const;

private:
	ErrorBufferInterface* m_errorhnd;
	const ScalarFunction* m_func;
	std::vector<double> m_valuear;
	std::size_t m_nof_args;
};

} //namespace
#endif


