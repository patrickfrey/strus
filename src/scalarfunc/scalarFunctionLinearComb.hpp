/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Constructor of a scalar function interface implementing a linear combination
/// \file "scalarFunctionLinearComb.hpp"
#ifndef _STRUS_STORAGE_SCALARFUNC_LINEARCOMB_HPP_INCLUDED
#define _STRUS_STORAGE_SCALARFUNC_LINEARCOMB_HPP_INCLUDED
#include <vector>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class ScalarFunctionInterface;

/// \brief Create an instance of a scalar function that implements a linear combination of the arguments
ScalarFunctionInterface* createScalarFunction_linearcomb( const std::vector<double>& factors, ErrorBufferInterface* errorhnd);

}//namespace
#endif

