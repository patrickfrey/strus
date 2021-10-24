/*
 * Copyright (c) 2021 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface of a storage of term and document occurrence statistics
/// \file statisticStorageInterface.hpp
#ifndef _STRUS_STATISTICS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STATISTICS_STORAGE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class StatisticsStorageClientInterface;

/// \brief Interface to the create and alter a storage of term and document occurrence statistics
class StatisticsStorageInterface
{
public:
	/// \brief Destructor
	virtual ~StatisticsStorageInterface(){}

	/// \brief Creates an client instance of the storage using a defined key value store database
	/// \param[in] configsource configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by this storage
	/// \param[in] statisticsProc defines the format of statistic messages (distribute statistics)
	virtual StatisticsStorageClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database,
			const StatisticsProcessorInterface* statisticsProc) const=0;

	/// \brief Creates a new storage described with configsource using a defined key value store database
	/// \param[in] configsource Configuration source string describing the storage and the database (not a filename !)
	/// \param[in] database key value store database type used by this storage
	/// \return true on success, false on error
	/// \remark The database referenced by 'database' must have been created and active
	virtual bool createStorage(
			const std::string& configsource,
			const DatabaseInterface* database) const=0;
};

}//namespace
#endif

