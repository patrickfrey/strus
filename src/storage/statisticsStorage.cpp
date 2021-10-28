/*
 * Copyright (c) 2021 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "statisticsStorage.hpp"
#include "statisticsStorageClient.hpp"
#include "strus/statisticsStorageInterface.hpp"
#include "strus/statisticsStorageClientInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/fileio.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/versionStorage.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "databaseAdapter.hpp"
#include "storage.hpp"
#include "byteOrderMark.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <memory>

using namespace strus;

StatisticsStorageClientInterface* StatisticsStorage::createClient(
		const std::string& configsource,
		const DatabaseInterface* database,
		const StatisticsProcessorInterface* statisticsProc) const
{
	try
	{
		if (m_errorhnd->hasError()) return 0;
		Reference<StatisticsStorageClient> client( new StatisticsStorageClient( database, statisticsProc, configsource, m_errorhnd));
		return client.release();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating statistics storage client: %s"), *m_errorhnd, 0);
}

bool StatisticsStorage::createStorage(
		const std::string& configsource,
		const DatabaseInterface* dbi) const
{
	try
	{
		ByteOrderMark byteOrderMark;

		std::string src = configsource;
		removeKeyFromConfigString( src, "statsproc", m_errorhnd);
		if (m_errorhnd->hasError()) return false;

		if (!dbi->createDatabase( src)) throw std::runtime_error( _TXT("failed to create key/value store database"));
		strus::local_ptr<strus::DatabaseClientInterface> database( dbi->createClient( src));
		if (!database.get()) throw std::runtime_error( _TXT("failed to create database client"));

		// 1st phase, store variables:
		strus::local_ptr<DatabaseTransactionInterface> transaction( database->createTransaction());
		if (!transaction.get()) return false;

		DatabaseAdapter_Variable<GlobalCounter>::Writer varstor_64( database.get());
		DatabaseAdapter_Variable<Index>::Writer varstor_32( database.get());

		varstor_32.store( transaction.get(), "TypeNo", 1);
		varstor_64.store( transaction.get(), "NofDocs", 0);
		varstor_32.store( transaction.get(), "ByteOrderMark", byteOrderMark.value());
		varstor_32.store( transaction.get(), "Version", (STRUS_STORAGE_VERSION_MAJOR * 1000) + STRUS_STORAGE_VERSION_MINOR);
		return transaction->commit();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage (physically): %s"), *m_errorhnd, false);
}

const char* StatisticsStorage::getConfigDescription() const
{
	return "";
}

const char** StatisticsStorage::getConfigParameters() const
{
	static const char* ar[] = {NULL};
	return ar;
}

