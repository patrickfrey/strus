/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus scalarfunc library
/// \file "scalarfunc.hpp"
#ifndef _STRUS_STORAGE_SCALARFUNC_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_SCALARFUNC_LIB_HPP_INCLUDED
#include <vector>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ScalarFunctionParserInterface;
/// \brief Forward declaration
class ScalarFunctionInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Create an instance of the default scalar function parser
ScalarFunctionParserInterface* createScalarFunctionParser_default( ErrorBufferInterface* errorhnd);

}//namespace
#endif

