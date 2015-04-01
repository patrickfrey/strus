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
#ifndef _STRUS_DATABASE_INTERFACE_HPP_INCLUDED
#define _STRUS_DATABASE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseBackupCursorInterface;

/// \brief Interface to the create,destroy the key value store database
class DatabaseInterface
{
public:
	/// \brief Creates a client to access the key value store database
	/// \param[in] configsource configuration source string describing the database (not a filename !)
	virtual DatabaseClientInterface* createClient( const std::string& configsource) const=0;

	/// \brief Creates a new key value store database described with config
	/// \param[in] configsource configuration source string describing the database (not a filename !)
	virtual void createDatabase( const std::string& configsource) const=0;

	/// \brief Restore a database at the location described in config and restore the backup from the backup interface passed
	/// \param[in] configsource configuration source string describing location of the database restored (not a filename !)
	/// \param[in] backup interface to backup to restore
	virtual void restoreDatabase( const std::string& configsource, DatabaseBackupCursorInterface* backup) const=0;

	/// \brief Destroys an existing key value store database described with config
	/// \param[in] configsource configuration source string describing the key value store database (not a filename !)
	virtual void destroyDatabase( const std::string& configsource) const=0;

	/// \brief Enumeration of different type of configurations
	///	Needed for getting the correct description of the configuration
	enum ConfigType
	{
		CmdCreateClient,		///< Config description for the creation of an instance accessing the repository
		CmdCreate,			///< Config description for the creation of a repository that does not exist yet
		CmdDestroy			///< Config description for the physical disposal of a repository
	};

	/// \brief Gets an example configuration description (source string as used by the functions here)
	///	createDatabase(const char*), destroyDatabase(const char*) and createDatabaseClient(const char*)
	///	for the usage printed by programs using this database.
	virtual const char* getConfigDescription( ConfigType type) const=0;

	/// \brief Get the list of known configuration parameter keys
	///	for verification of the configuration by programs using this database.
	virtual const char** getConfigParameters( ConfigType type) const=0;
};

}//namespace
#endif

