/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library with some convenient functions to create storage objects
/// \file "storage_objbuild.cpp"
#include "strus/lib/storage_objbuild.hpp"
#include "strus/lib/queryeval.hpp"
#include "strus/lib/queryproc.hpp"
#include "strus/lib/statsproc.hpp"
#include "strus/lib/storage.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "strus/storageObjectBuilderInterface.hpp"
#include "strus/storageAlterMetaDataTableInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/base/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/utils.hpp"
#include "strus/base/configParser.hpp"
#include <memory>

using namespace strus;

static bool g_intl_initialized = false;

/// \brief Implementation of the default StorageObjectBuilderInterface (no module loading)
class StorageObjectBuilder
	:public StorageObjectBuilderInterface
{
public:
	explicit StorageObjectBuilder( ErrorBufferInterface* errorhnd_)
		:m_queryProcessor( strus::createQueryProcessor(errorhnd_))
		,m_storage(strus::createStorage(errorhnd_))
		,m_db( strus::createDatabase_leveldb( errorhnd_))
		,m_statsproc( strus::createStatisticsProcessor( errorhnd_))
		,m_errorhnd(errorhnd_)
	{
		if (!m_queryProcessor.get()) throw strus::runtime_error(_TXT("error creating query processor"));
		if (!m_storage.get()) throw strus::runtime_error(_TXT("error creating default storage"));
		if (!m_db.get()) throw strus::runtime_error(_TXT("error creating default database '%s'"), "leveldb");
		if (!m_statsproc.get()) throw strus::runtime_error(_TXT("error creating default statistics processor"));
	}

	virtual ~StorageObjectBuilder(){}

	virtual const StorageInterface* getStorage() const
	{
		return m_storage.get();
	}
	virtual const DatabaseInterface* getDatabase( const std::string&) const
	{
		return m_db.get();
	}
	virtual const QueryProcessorInterface* getQueryProcessor() const
	{
		return m_queryProcessor.get();
	}
	virtual const StatisticsProcessorInterface* getStatisticsProcessor( const std::string& name) const
	{
		try
		{
			if (name.empty() || utils::tolower( name) == "default")
			{
				return m_statsproc.get();
			}
			else
			{
				throw strus::runtime_error(_TXT("unknown statistics processor: '%s'"), name.c_str());
			}
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error getting statistics processor: %s"), *m_errorhnd, 0);
	}
	virtual QueryEvalInterface* createQueryEval() const
	{
		return strus::createQueryEval( m_errorhnd);
	}

private:
	Reference<QueryProcessorInterface> m_queryProcessor;	///< query processor handle
	Reference<StorageInterface> m_storage;			///< storage handle
	Reference<DatabaseInterface> m_db;			///< database handle
	Reference<StatisticsProcessorInterface> m_statsproc;	///< statistics processor handle
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};


DLL_PUBLIC StorageObjectBuilderInterface*
	strus::createStorageObjectBuilder_default(
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new StorageObjectBuilder( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating default storage object builder: %s"), *errorhnd, 0);
}


DLL_PUBLIC StorageAlterMetaDataTableInterface*
	strus::createAlterMetaDataTable(
		const StorageObjectBuilderInterface* objbuilder,
		ErrorBufferInterface* errorhnd,
		const std::string& config)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}

		std::string dbname;
		std::string configstr( config);

		const DatabaseInterface* dbi = objbuilder->getDatabase( configstr);
		const StorageInterface* sti = objbuilder->getStorage();

		(void)strus::extractStringFromConfigString( dbname, configstr, "database", errorhnd);
		std::string databasecfg( configstr);
		strus::removeKeysFromConfigString(
				databasecfg,
				sti->getConfigParameters( strus::StorageInterface::CmdCreateClient), errorhnd);
		//... In storagecfg is now the pure storage configuration without the database settings
		if (errorhnd->hasError())
		{
			errorhnd->explain(_TXT("cannot evaluate database: %s"));
			return 0;
		}
		std::auto_ptr<DatabaseClientInterface> database( dbi->createClient( databasecfg));
		if (!database.get())
		{
			errorhnd->report(_TXT("error creating database client"));
			return 0;
		}
		std::auto_ptr<StorageAlterMetaDataTableInterface> altermetatable( sti->createAlterMetaDataTable( database.get()));
		if (!altermetatable.get())
		{
			errorhnd->report(_TXT("error creating alter metadata table client"));
			return 0;
		}
		(void)database.release(); //... ownership passed to alter metadata table client
		return altermetatable.release(); //... ownership returned
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage alter metadata table interface: %s"), *errorhnd, 0);
}


DLL_PUBLIC StorageClientInterface*
	strus::createStorageClient(
		const StorageObjectBuilderInterface* objbuilder,
		ErrorBufferInterface* errorhnd,
		const std::string& config)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}

		std::string statsprocname;
		std::string dbname;
		std::string configstr( config);
	
		const DatabaseInterface* dbi = objbuilder->getDatabase( configstr);
		if (!dbi)
		{
			errorhnd->explain(_TXT("could not get database: %s"));
			return 0;
		}
		(void)strus::extractStringFromConfigString( dbname, configstr, "database", errorhnd);
		if (strus::extractStringFromConfigString( statsprocname, configstr, "statsproc", errorhnd))
		{
			if (statsprocname.empty())
			{
				statsprocname = "default";
			}
		}

		const StorageInterface* sti = objbuilder->getStorage();
		if (!sti)
		{
			errorhnd->explain(_TXT("could not get storage: %s"));
			return 0;
		}
		std::string databasecfg( configstr);
		std::string storagecfg( configstr);
		strus::removeKeysFromConfigString(
				databasecfg,
				sti->getConfigParameters( strus::StorageInterface::CmdCreateClient), errorhnd);
	
		strus::removeKeysFromConfigString(
				storagecfg,
				dbi->getConfigParameters( strus::DatabaseInterface::CmdCreateClient), errorhnd);
		//... In storagecfg is now the pure storage configuration without the database settings
		if (errorhnd->hasError())
		{
			errorhnd->explain(_TXT("cannot create database client: %s"));
			return 0;
		}
		std::auto_ptr<DatabaseClientInterface> database( dbi->createClient( databasecfg));
		if (!database.get())
		{
			errorhnd->report(_TXT("error creating database client"));
			return 0;
		}
		const StatisticsProcessorInterface* statsproc = 0;
		if (!statsprocname.empty())
		{
			statsproc = objbuilder->getStatisticsProcessor( statsprocname);
			if (!statsproc)
			{
				errorhnd->report(_TXT("error getting statistics message processor"));
				return 0;
			}
		}
		std::auto_ptr<StorageClientInterface>
			storage( sti->createClient( storagecfg, database.get(), statsproc));
		if (!storage.get())
		{
			errorhnd->report(_TXT("error creating storage client"));
			return 0;
		}
		(void)database.release(); //... ownership passed to storage
		return storage.release(); //... ownership returned
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *errorhnd, 0);
}


