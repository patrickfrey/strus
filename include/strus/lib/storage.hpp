/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus storage library
/// \file storage.hpp
#ifndef _STRUS_STORAGE_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_LIB_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageInterface;

/// \brief Create an interface for a strus storage, standard implementation
/// \param[in] workdir working directory where storage files are stored relatively to
/// \param[in] errorhnd error buffer interface
StorageInterface* createStorageType_std( const std::string& workdir, ErrorBufferInterface* errorhnd);

}//namespace
#endif

