/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Exported functions of the strus storage library
#ifndef _STRUS_STORAGE_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_LIB_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class StorageDatabaseInterface;
/// \brief Forward declaration
class StorageAlterMetaDataTableInterface;

/// \remark Because of restrictions imposed by LevelDB only one client of a storage instance can exist simultaneously. But this can be used by several threads.

/// \brief Creates an client instance of the storage using a defined key value store database
/// \param[in] configsource configuration source string describing the storage (not a filename !)
/// \param[in] database reference to key value store database used by this storage
StorageInterface* createStorageClient( const std::string& configsource, DatabaseInterface* database);

/// \brief Creates a new storage described with configsource using a defined key value store database
/// \param[in] configsource Configuration source string describing the storage (not a filename !)
/// \param[in] database reference to a key value store database used by the storage
/// \remark The database referenced by 'database' must have been created and active
void createStorage( const std::string& configsource, DatabaseInterface* database);

/// \brief Create an interface to alter the meta data table structure
/// \param[in] database reference to a key value store database used by the storage
/// \return the created reference to be disposed with delete by the caller
StorageAlterMetaDataTableInterface* createAlterMetaDataTable( DatabaseInterface* database);

/// \brief Selection of command types for the storage config description
enum StorageConfigDescriptionType
{
	CmdCreateStorageClient=0x1,	///< Config description for createStorageClient(const char*,DatabaseInterface*)
	CmdCreateStorage=0x2		///< Config description for createStorage(const char*,DatabaseInterface*)
};

/// \brief Gets an example configuration description (source string as used by the functions here)
///	createStorageDatabase(const char*) and createStorageClient(const char*)
///	for the usage printed by programs using this storage implementation.
const char* getStorageConfigDescription( StorageConfigDescriptionType type);

/// \brief Get the list of known configuration parameter keys
///	for verification of the configuration by programs using this storage implementation.
const char** getStorageConfigParameters( StorageConfigDescriptionType type);

}//namespace
#endif

