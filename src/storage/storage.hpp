/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STORAGE_IMPLEMENTATION_HPP_INCLUDED
#include "strus/storageInterface.hpp"
#include <string>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class StorageAlterMetaDataTableInterface;
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Object to the create and alter a storage for strus
class Storage
	:public StorageInterface
{
public:
	explicit Storage( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual StorageClientInterface* createClient(
			const std::string& configsource,
			const DatabaseInterface* database,
			const StatisticsProcessorInterface* statisticsProc) const;

	virtual bool createStorage(
			const std::string& configsource,
			const DatabaseInterface* database) const;
	
	virtual StorageAlterMetaDataTableInterface* createAlterMetaDataTable(
			const std::string& configsource,
			const DatabaseInterface* database) const;

	virtual const char* getConfigDescription( const ConfigType& type) const;

	virtual const char** getConfigParameters( const ConfigType& type) const;

	virtual StorageDumpInterface* createDump(
			const std::string& configsource,
			const DatabaseInterface* database,
			const std::string& keyprefix) const;

private:
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif

