/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Toplevel interface of a storage
/// \file storageInterface.hpp
#ifndef _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class StorageAlterMetaDataTableInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;


/// \brief Interface to the create and alter a storage for strus
class StorageInterface
{
public:
	/// \brief Destructor
	virtual ~StorageInterface(){}

	/// \brief Creates an client instance of the storage using a defined key value store database
	/// \param[in] configsource configuration source string describing the storage (not a filename !)
	/// \param[in] database key value store database used by this storage (ownership passed to returned object)
	/// \param[in] (optional) statisticsProc defines the format of statistic messages (distribute statistics)
	virtual StorageClientInterface* createClient(
			const std::string& configsource,
			DatabaseClientInterface* database,
			const StatisticsProcessorInterface* statisticsProc=0) const=0;

	/// \brief Creates a new storage described with configsource using a defined key value store database
	/// \param[in] configsource Configuration source string describing the storage (not a filename !)
	/// \param[in] database reference to a key value store database used by the storage
	/// \return true on success, false on error
	/// \remark The database referenced by 'database' must have been created and active
	virtual bool createStorage(
			const std::string& configsource,
			DatabaseClientInterface* database) const=0;

	/// \brief Create an interface to alter the meta data table structure
	/// \param[in] database key value store database used by the storage (ownership passed to returned object)
	/// \return the created reference to be disposed with delete by the caller
	virtual StorageAlterMetaDataTableInterface* createAlterMetaDataTable(
			DatabaseClientInterface* database) const=0;

	/// \brief Enumeration of different type of configurations
	///	Needed for getting the correct description of the configuration
	enum ConfigType
	{
		CmdCreateClient,		///< Config description for the creation of an instance accessing the repository
		CmdCreate			///< Config description for the creation of a repository that does not exist yet
	};

	/// \brief Gets a configuration description (source string as used by the functions here)
	///	createStorage(const std::string&) and createClient(const std::string&)
	///	for the usage printed by programs using this storage implementation.
	virtual const char* getConfigDescription( const ConfigType& type) const=0;

	/// \brief Get the list of known configuration parameter keys
	///	for verification of the configuration by programs using this storage implementation.
	virtual const char** getConfigParameters( const ConfigType& type) const=0;
};

}//namespace
#endif

