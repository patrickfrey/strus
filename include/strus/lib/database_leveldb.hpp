/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the library implementing the key/value store database interface with leveldb
/// \file database_leveldb.hpp
#ifndef _STRUS_DATABASE_LEVELDB_LIB_HPP_INCLUDED
#define _STRUS_DATABASE_LEVELDB_LIB_HPP_INCLUDED
#include <string>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class FileLocatorInterface;
/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Create a database interface implemented with leveldb with the functions for accessing the key/value store database.
/// \param[in] filelocator interface to locate files to read or the working directory where to write files to
/// \param[in] errorhnd reference to error buffer (ownership hold by caller)
DatabaseInterface* createDatabaseType_leveldb( const FileLocatorInterface* filelocator, ErrorBufferInterface* errorhnd);

}//namespace
#endif

