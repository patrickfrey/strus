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
#include "strus/lib/statsstorage.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/constants.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/reference.hpp"
#include "strus/storageObjectBuilderInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/statisticsStorageInterface.hpp"
#include "strus/statisticsStorageClientInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/statisticsProcessorInterface.hpp"
#include "strus/vectorStorageInterface.hpp"
#include "strus/vectorStorageClientInterface.hpp"
#include "strus/base/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include <memory>

using namespace strus;

static bool g_intl_initialized = false;

/// \brief Implementation of the default StorageObjectBuilderInterface (no module loading)
class StorageObjectBuilder
	:public StorageObjectBuilderInterface
{
public:
	explicit StorageObjectBuilder( const FileLocatorInterface* filelocator_, ErrorBufferInterface* errorhnd_)
		:m_queryProcessor( strus::createQueryProcessor(filelocator_,errorhnd_))
		,m_storage(strus::createStorageType_std( filelocator_, errorhnd_))
		,m_statstorage(strus::createStatisticsStorageType_std( filelocator_, errorhnd_))
		,m_db( strus::createDatabaseType_leveldb( filelocator_, errorhnd_))
		,m_statsproc( strus::createStatisticsProcessor_std( filelocator_, errorhnd_))
		,m_errorhnd(errorhnd_)
		,m_filelocator(filelocator_)
	{
		if (!m_queryProcessor.get()) throw std::runtime_error( _TXT("error creating query processor"));
		if (!m_storage.get()) throw std::runtime_error( _TXT("error creating default storage"));
		if (!m_statstorage.get()) throw std::runtime_error( _TXT("error creating default statistics storage"));
		if (!m_db.get()) throw strus::runtime_error(_TXT("error creating default database '%s'"), "leveldb");
		if (!m_statsproc.get()) throw std::runtime_error( _TXT("error creating default statistics processor"));
	}

	virtual ~StorageObjectBuilder(){}

	virtual const StorageInterface* getStorage() const
	{
		return m_storage.get();
	}
	virtual const StatisticsStorageInterface* getStatisticsStorage() const
	{
		return m_statstorage.get();
	}
	virtual const DatabaseInterface* getDatabase( const std::string& name) const
	{
		try
		{
			if (name.empty() || string_conv::tolower( name) == strus::Constants::leveldb_database_name())
			{
				return m_db.get();
			}
			else
			{
				throw strus::runtime_error(_TXT("unknown database interface: '%s'"), name.c_str());
			}
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error getting database interface: %s"), *m_errorhnd, 0);
	}
	virtual const QueryProcessorInterface* getQueryProcessor() const
	{
		return m_queryProcessor.get();
	}
	virtual const StatisticsProcessorInterface* getStatisticsProcessor( const std::string& name) const
	{
		try
		{
			if (name.empty() || string_conv::tolower( name) == strus::Constants::standard_statistics_processor())
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
	virtual const VectorStorageInterface* getVectorStorage( const std::string& name) const
	{
		m_errorhnd->report( ErrorCodeNotFound, _TXT("unknown vector storage interface: '%s'"), name.c_str());
		return 0;
	}
	virtual QueryEvalInterface* createQueryEval() const
	{
		return strus::createQueryEval( m_errorhnd);
	}

private:
	Reference<QueryProcessorInterface> m_queryProcessor;	///< query processor handle
	Reference<StorageInterface> m_storage;			///< storage handle
	Reference<StatisticsStorageInterface> m_statstorage;	///< statistics storage handle
	Reference<DatabaseInterface> m_db;			///< database handle
	Reference<StatisticsProcessorInterface> m_statsproc;	///< statistics processor handle
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
	const FileLocatorInterface* m_filelocator;		///< file locator interface
};


DLL_PUBLIC StorageObjectBuilderInterface*
	strus::createStorageObjectBuilder_default(
		const FileLocatorInterface* filelocator,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		if (!g_intl_initialized)
		{
			strus::initMessageTextDomain();
			g_intl_initialized = true;
		}
		return new StorageObjectBuilder( filelocator, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating default storage object builder: %s"), *errorhnd, 0);
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
		(void)strus::extractStringFromConfigString( dbname, configstr, "database", errorhnd);

		const DatabaseInterface* dbi = objbuilder->getDatabase( dbname);
		if (!dbi)
		{
			errorhnd->explain(_TXT("could not get database: %s"));
			return 0;
		}
		if (strus::extractStringFromConfigString( statsprocname, configstr, "statsproc", errorhnd))
		{
			if (statsprocname.empty())
			{
				statsprocname = strus::Constants::standard_statistics_processor();
			}
		}

		const StorageInterface* sti = objbuilder->getStorage();
		if (!sti)
		{
			errorhnd->explain(_TXT("could not get storage: %s"));
			return 0;
		}
		const StatisticsProcessorInterface* statsproc = 0;
		if (!statsprocname.empty())
		{
			statsproc = objbuilder->getStatisticsProcessor( statsprocname);
			if (!statsproc)
			{
				errorhnd->explain( _TXT("error getting statistics message processor: %s"));
				return 0;
			}
		}
		strus::local_ptr<StorageClientInterface>
			storage( sti->createClient( configstr, dbi, statsproc));
		if (!storage.get())
		{
			return 0;
		}
		return storage.release(); //... ownership returned
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *errorhnd, 0);
}

DLL_PUBLIC StatisticsStorageClientInterface*
	strus::createStatisticsStorageClient(
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
		(void)strus::extractStringFromConfigString( dbname, configstr, "database", errorhnd);

		const DatabaseInterface* dbi = objbuilder->getDatabase( dbname);
		if (!dbi)
		{
			errorhnd->explain(_TXT("could not get database: %s"));
			return 0;
		}
		if (strus::extractStringFromConfigString( statsprocname, configstr, "statsproc", errorhnd))
		{
			if (statsprocname.empty())
			{
				statsprocname = strus::Constants::standard_statistics_processor();
			}
		}
		const StatisticsStorageInterface* sti = objbuilder->getStatisticsStorage();
		if (!sti)
		{
			errorhnd->explain(_TXT("could not get statistics storage: %s"));
			return 0;
		}
		const StatisticsProcessorInterface* statsproc = 0;
		if (!statsprocname.empty())
		{
			statsproc = objbuilder->getStatisticsProcessor( statsprocname);
			if (!statsproc)
			{
				errorhnd->explain( _TXT("error getting statistics message processor: %s"));
				return 0;
			}
		}
		if (!statsproc)
		{
			throw std::runtime_error(_TXT("missing mandatory statistics processor definition"));
		}
		strus::local_ptr<StatisticsStorageClientInterface> storage( sti->createClient( configstr, dbi, statsproc));
		if (!storage.get())
		{
			return 0;
		}
		return storage.release(); //... ownership returned
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *errorhnd, 0);
}

DLL_PUBLIC VectorStorageClientInterface*
	strus::createVectorStorageClient(
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
		std::string storagename;
		std::string configstr( config);
		(void)strus::extractStringFromConfigString( dbname, configstr, "database", errorhnd);
		if (!strus::extractStringFromConfigString( storagename, configstr, "storage", errorhnd))
		{
			storagename = strus::Constants::standard_vector_storage();
		}
		const DatabaseInterface* dbi = objbuilder->getDatabase( dbname);
		if (!dbi)
		{
			errorhnd->explain(_TXT("could not get database: %s"));
			return 0;
		}
		const VectorStorageInterface* sti = objbuilder->getVectorStorage( storagename);
		if (!sti)
		{
			errorhnd->explain(_TXT("could not get storage: %s"));
			return 0;
		}
		strus::local_ptr<VectorStorageClientInterface>
			storage( sti->createClient( configstr, dbi));
		if (!storage.get())
		{
			errorhnd->explain( _TXT("error creating vector storage client: %s"));
			return 0;
		}
		return storage.release(); //... ownership returned
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating vector storage client: %s"), *errorhnd, 0);
}

