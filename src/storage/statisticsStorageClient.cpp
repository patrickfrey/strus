/*
 * Copyright (c) 2021 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "statisticsStorageClient.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/statisticsViewerInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/versionStorage.hpp"
#include "strus/reference.hpp"
#include "strus/lib/error.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/unordered_map.hpp"
#include "strus/base/configParser.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/databaseClientUndefinedStub.hpp"
#include "databaseKey.hpp"
#include "byteOrderMark.hpp"
#include "extractKeyValueData.hpp"
#include "databaseAdapter.hpp"
#include "indexPacker.hpp"
#include "keyAllocatorInterface.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

using namespace strus;

#define MODULENAME "statisticsStorageClient"

static char const** getConfigParamList( const DatabaseInterface* db);

StatisticsStorageClient::StatisticsStorageClient(
		const DatabaseInterface* database_,
		const StatisticsProcessorInterface* statisticsProc_,
		const std::string& databaseConfig,
		ErrorBufferInterface* errorhnd_)
	:m_dbtype(database_)
	,m_database()
	,m_cfgparam(getConfigParamList(database_))
	,m_next_typeno(0)
	,m_nof_documents(0)
	,m_close_called(false)
	,m_statisticsProc(statisticsProc_)
	,m_errorhnd(errorhnd_)
{
	init( databaseConfig);
}

StatisticsStorageClient::~StatisticsStorageClient()
{
	if (!m_close_called) try
	{
		storeVariables();
	}
	CATCH_ERROR_MAP( _TXT("error closing storage client: %s"), *m_errorhnd);
	std::free( m_cfgparam);
}

static char const** getConfigParamList( const DatabaseInterface* db)
{
	char const** rt;
	std::vector<const char*> cfgar;
	char const** cfg = db->getConfigParameters();
	for (int ci = 0; cfg[ci]; ++ci) cfgar.push_back( cfg[ci]);
	cfgar.push_back( "database");
	rt = (char const**)std::malloc( (cfgar.size()+1) * sizeof(rt[0]));
	if (rt == NULL) throw std::bad_alloc();
	std::memcpy( rt, cfgar.data(), cfgar.size() * sizeof(rt[0]));
	rt[ cfgar.size()] = 0;
	return rt;
}

const char** StatisticsStorageClient::getConfigParameters() const
{
	return m_cfgparam;
}

void StatisticsStorageClient::init( const std::string& databaseConfig)
{
	std::string databaseConfigCopy( databaseConfig);

	Reference<DatabaseClientInterface> db( m_dbtype->createClient( databaseConfigCopy));
	if (!db.get()) throw strus::runtime_error(_TXT("failed to initialize database client: %s"), m_errorhnd->fetchError());

	m_database = db;
	m_close_called = false;
	loadVariables( m_database.get());
}

bool StatisticsStorageClient::reload( const std::string& databaseConfig)
{
	try
	{
		std::string src = databaseConfig;
		std::string statsproc;

		(void)removeKeyFromConfigString( src, "database", m_errorhnd);

		if (m_errorhnd->hasError()) return false;

		close();
		m_next_typeno.set(0);
		m_nof_documents.set(0);

		m_database.reset( new DatabaseClientUndefinedStub( m_errorhnd));
		//... the assignment of DatabaseClientUndefinedStub guarantees that m_database is initialized, event if 'init' throws

		init( databaseConfig);
		return true;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' reloading configuration: %s"), MODULENAME, *m_errorhnd, false);
}

long StatisticsStorageClient::diskUsage() const
{
	return m_database->diskUsage();
}

std::string StatisticsStorageClient::config() const
{
	try
	{
		std::string rt( m_database->config());
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' mapping configuration to string: %s"), MODULENAME, *m_errorhnd, std::string());
}

static Index versionNo( Index major, Index minor)
{
	return (major * 1000) + minor;
}

void StatisticsStorageClient::loadVariables( DatabaseClientInterface* database_)
{
	ByteOrderMark byteOrderMark;
	Index bom;
	ByteOrderMark storage_byteOrderMark;
	Index next_typeno_;
	GlobalCounter nof_documents_;
	Index version_;

	DatabaseAdapter_Variable<GlobalCounter>::Reader varstor_64( database_);
	DatabaseAdapter_Variable<Index>::Reader varstor_32( database_);
	if (!varstor_32.load( "TypeNo", next_typeno_))
	{
		throw strus::runtime_error( "%s",  _TXT( "corrupt storage, not all mandatory variables defined"));
	}
	if (!varstor_64.load( "NofDocs", nof_documents_))
	{
		throw strus::runtime_error( "%s",  _TXT( "corrupt storage, not all mandatory variables defined"));
	}
	if (!varstor_32.load( "Version", version_))
	{
		version_ = versionNo( 0, 4);
	}
	if (version_ / 1000 != STRUS_STORAGE_VERSION_MAJOR || version_ / 1000 > STRUS_STORAGE_VERSION_MINOR)
	{
		unsigned int major = version_ / 1000;
		unsigned int minor = version_ % 1000;
		throw strus::runtime_error( _TXT( "incompatible storage version %u.%u software is %u.%u."), major, minor, (unsigned int)STRUS_STORAGE_VERSION_MAJOR, (unsigned int)STRUS_STORAGE_VERSION_MINOR);
	}
	if (varstor_32.load( "ByteOrderMark", bom))
	{
		if (bom != byteOrderMark.value())
		{
			storage_byteOrderMark.set( bom);
			throw strus::runtime_error( _TXT( "incompatible platform for accessing this storage, storage created as %s, but accessed from a machine with %s"), storage_byteOrderMark.endianess(), byteOrderMark.endianess());
		}
	}
	m_next_typeno.set( next_typeno_);
	m_nof_documents.set( nof_documents_);
}

void StatisticsStorageClient::storeVariables()
{
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
	if (!transaction.get()) throw std::runtime_error( _TXT("error storing variables"));
	getVariablesWriteBatch( transaction.get(), 0);
	transaction->commit();
}

void StatisticsStorageClient::getVariablesWriteBatch(
		DatabaseTransactionInterface* transaction,
		int nof_documents_incr)
{
	DatabaseAdapter_Variable<GlobalCounter>::Writer varstor_64( m_database.get());
	DatabaseAdapter_Variable<Index>::Writer varstor_32( m_database.get());
	varstor_32.store( transaction, "TypeNo", m_next_typeno.value());
	varstor_64.store( transaction, "NofDocs", m_nof_documents.value() + nof_documents_incr);
}

Index StatisticsStorageClient::getTermType( const std::string& name) const
{
	try
	{
		return DatabaseAdapter_TermType::Reader( m_database.get()).get( string_conv::tolower( name));
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error evaluating term type: %s"), *m_errorhnd, 0);
}

Index StatisticsStorageClient::allocTypenoImm( const std::string& name)
{
	Index rt;
	DatabaseAdapter_TermType::ReadWriter stor(m_database.get());

	strus::scoped_lock lock( m_immalloc_typeno_mutex);
	if (!stor.load( name, rt))
	{
		stor.storeImm( name, rt = m_next_typeno.allocIncrement());
	}
	return rt;
}

GlobalCounter StatisticsStorageClient::nofDocuments() const
{
	return m_nof_documents.value();
}

GlobalCounter StatisticsStorageClient::documentFrequency( const std::string& type, const std::string& term) const
{
	try
	{
		Index typeno = getTermType( type);
		if (typeno)
		{
			return DatabaseAdapter_DocFrequencyStatistics::get( m_database.get(), typeno, term);
		}
		else
		{
			return 0;
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error evaluating term document frequency: %s"), *m_errorhnd, 0);
}

TimeStamp StatisticsStorageClient::storageTimeStamp( const std::string& storageid) const
{
	try {
		DatabaseAdapter_StorageTimestamp::Reader reader( m_database.get());
		return reader.get( storageid, -1);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error getting a storage timestamp: %s"), *m_errorhnd, 0);
}

bool StatisticsStorageClient::putStatisticsMessage( const StatisticsMessage& msg, const std::string& storageid)
{
	try {
		strus::Reference<StatisticsViewerInterface> viewer( m_statisticsProc->createViewer( msg.ptr(), msg.size()));
		if (!viewer.get()) return false;
		int nofDocumentsDiff = viewer->nofDocumentsInsertedChange();
		TermStatisticsChange rec;
		std::map<std::string,Index> typenoCache;

		strus::scoped_lock lock( m_transaction_mutex);
		Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());
		if (!transaction.get()) return false;

		// Update list of term df changes:
		while (viewer->nextDfChange( rec))
		{
			std::string typestr( strus::string_conv::tolower( rec.type()));
			auto ti = typenoCache.find( typestr);
			Index typeno;
			if (ti == typenoCache.end())
			{
				typeno = allocTypenoImm( typestr);
				typenoCache.insert( {typestr,typeno});
			}
			else
			{
				typeno = ti->second;
			}
			std::string term( rec.value());
			GlobalCounter df = DatabaseAdapter_DocFrequencyStatistics::get( m_database.get(), typeno, term);
			df += rec.increment();

			DatabaseAdapter_DocFrequencyStatistics::store( transaction.get(), typeno, term, df);
		}
		// Update TimeStamp:
		DatabaseAdapter_StorageTimestamp::ReadWriter timestamp_db( m_database.get());
		TimeStamp oldTimeStamp = timestamp_db.get( storageid, -1);
		TimeStamp newTimeStamp = msg.timestamp();
		if (newTimeStamp > oldTimeStamp)
		{
			timestamp_db.store( transaction.get(), storageid, newTimeStamp);
		}
		// Update NofDocuments:
		getVariablesWriteBatch( transaction.get(), nofDocumentsDiff);
		if (transaction->commit())
		{
			m_nof_documents.increment( nofDocumentsDiff);
			return true;
		}
		else
		{
			return false;
		}
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in put statistics message: %s"), *m_errorhnd, 0);
}

const StatisticsProcessorInterface* StatisticsStorageClient::getStatisticsProcessor() const
{
	return m_statisticsProc;
}

void StatisticsStorageClient::close()
{
	try
	{
		if (!m_close_called)
		{
			storeVariables();
			m_database->compactDatabase();
			m_database->close();
			m_close_called = true;
		}
	}
	CATCH_ERROR_MAP( _TXT("error storing variables in close of storage: %s"), *m_errorhnd);
}

void StatisticsStorageClient::compaction()
{
	try
	{
		storeVariables();
		m_database->compactDatabase();
	}
	CATCH_ERROR_MAP( _TXT("error in compaction of storage: %s"), *m_errorhnd);
}

