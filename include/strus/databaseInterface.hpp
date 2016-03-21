/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Toplevel interface for the key/value store database
/// \file "databaseInterface.hpp"
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
	/// \brief Destructor
	virtual ~DatabaseInterface(){}

	/// \brief Evaluate if the database specified exists
	/// \param[in] configsource configuration source string describing the database (not a filename !)
	virtual bool exists( const std::string& configsource) const=0;

	/// \brief Creates a client to access the key value store database
	/// \param[in] configsource configuration source string describing the database (not a filename !)
	/// \return the client on success or NULL, if the operation failed
	virtual DatabaseClientInterface* createClient( const std::string& configsource) const=0;

	/// \brief Creates a new key value store database described with config
	/// \param[in] configsource configuration source string describing the database (not a filename !)
	/// \return true, if the operation succeeded
	virtual bool createDatabase( const std::string& configsource) const=0;

	/// \brief Restore a database at the location described in config and restore the backup from the backup interface passed
	/// \param[in] configsource configuration source string describing location of the database restored (not a filename !)
	/// \param[in] backup interface to backup to restore
	/// \return true, if the operation succeeded
	virtual bool restoreDatabase( const std::string& configsource, DatabaseBackupCursorInterface* backup) const=0;

	/// \brief Destroys an existing key value store database described with config
	/// \param[in] configsource configuration source string describing the key value store database (not a filename !)
	/// \return true, if the operation succeeded
	virtual bool destroyDatabase( const std::string& configsource) const=0;

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

