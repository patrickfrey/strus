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
/// \brief Exported functions of the strus key value store database library
#ifndef _STRUS_DATABASE_LIB_HPP_INCLUDED
#define _STRUS_DATABASE_LIB_HPP_INCLUDED

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;

/// \brief Creates an instance of the key value store database interface
/// \param[in] configsource configuration source string describing the database (not a filename !)
/// \remark Because of restrictions imposed by LevelDB only one client of a key value store database instance can exist simultaneously. But this one can be used by several threads at the same time.
DatabaseInterface* createDatabaseClient( const char* configsource);

/// \brief Creates a new key value store database described with config
/// \param[in] configsource configuration source string describing the database (not a filename !)
void createDatabase( const char* configsource);

/// \brief Destroys an existing key value store database described with config in the file system
/// \param[in] configsource configuration source string describing the key value store database (not a filename !)
void destroyDatabase( const char* configsource);

/// \brief Selection of command types for the storage config description
enum DatabaseConfigDescriptionType
{
	CmdCreateDatabaseClient=0x1,	///< Config description for createDatabaseClient(const char*)
	CmdCreateDatabase=0x2,		///< Config description for createDatabase(const char*)
	CmdDestroyDatabase=0x4		///< Config description for destroyDatabase(const char*)
};

/// \brief Gets an example configuration description (source string as used by the functions here)
///	createDatabase(const char*), destroyDatabase(const char*) and createDatabaseClient(const char*)
///	for the usage printed by programs using this database.
const char* getDatabaseConfigDescription( DatabaseConfigDescriptionType type);

/// \brief Get the list of known configuration parameter keys
///	for verification of the configuration by programs using this database.
const char** getDatabaseConfigParameters( DatabaseConfigDescriptionType type);

}//namespace
#endif

