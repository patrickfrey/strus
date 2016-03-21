/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus query evaluation library
/// \file "queryeval.hpp"
#ifndef _STRUS_QUERYEVAL_LIB_HPP_INCLUDED
#define _STRUS_QUERYEVAL_LIB_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class QueryEvalInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create a parameterizable instance of a query evaluation scheme
/// \return the program reference
QueryEvalInterface* createQueryEval( ErrorBufferInterface* errorhnd);

}//namespace
#endif

