/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface of a storage of a search index
/// \file storageInterface.hpp
#ifndef _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class StorageDumpInterface;


/// \brief Interface to the create and alter a storage of a search index
class StorageInterface
{
public:
	/// \brief Destructor
	virtual ~StorageInterface(){}

	/// \brief Creates an client instance of the storage using a defined key value store database
	/// \param[in] configsource configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by this storage
	/// \param[in] (optional) statisticsProc defines the format of statistic messages (distribute statistics)
	virtual StorageClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database,
			const StatisticsProcessorInterface* statisticsProc=0) const=0;

	/// \brief Creates a new storage described with configsource using a defined key value store database
	/// \param[in] configsource Configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by this storage
	/// \return true on success, false on error
	/// \remark The database referenced by 'database' must have been created and active
	virtual bool createStorage(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;

	/// \brief Gets a configuration description (source string as used by the functions here)
	///	for the usage printed by programs using this storage implementation.
	virtual const char* getConfigDescription() const=0;

	/// \brief Get the list of known configuration parameter keys
	///	for verification of the configuration by programs using this storage implementation.
	virtual const char** getConfigParameters() const=0;
};

}//namespace
#endif

