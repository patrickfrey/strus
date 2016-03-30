/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus error library implementing the standard error buffer interface
/// \file error.hpp
#ifndef _STRUS_STORAGE_ERROR_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_ERROR_LIB_HPP_INCLUDED
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

ErrorBufferInterface* createErrorBuffer_standard( FILE* logfilehandle, std::size_t maxNofThreads_);

}//namespace
#endif

