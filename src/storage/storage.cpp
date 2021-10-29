/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storage.hpp"
#include "storageClient.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDumpInterface.hpp"
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

StorageClientInterface* Storage::createClient(
		const std::string& configsource,
		const DatabaseInterface* database,
		const StatisticsProcessorInterface* statisticsProc) const
{
	try
	{
		if (m_errorhnd->hasError()) return 0;
		std::string cachedtermspath;
		std::string databaseConfig = configsource;
		bool hasCachedTerms = strus::extractStringFromConfigString( cachedtermspath, databaseConfig, "cachedterms", m_errorhnd);

		Reference<StorageClient> client( new StorageClient( database, statisticsProc, databaseConfig, m_errorhnd));

		if (hasCachedTerms)
		{
			std::string cachedtermsfile = m_filelocator->getResourceFilePath( cachedtermspath);
			if (m_errorhnd->hasError()) {m_errorhnd->explain( _TXT( "could not locate cached terms file")); return 0;}

			std::string cachedtermsrc;
			int ec = strus::readFile( cachedtermsfile, cachedtermsrc);
			if (ec) throw strus::runtime_error( _TXT( "could not read file '%s': %s"), cachedtermsfile.c_str(), ::strerror(ec));

			client->loadTermnoMap( cachedtermsrc.c_str());
		}
		return client.release();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *m_errorhnd, 0);
}

bool Storage::createStorage(
		const std::string& configsource,
		const DatabaseInterface* dbi) const
{
	try
	{
		bool useAcl = false;
		ByteOrderMark byteOrderMark;

		std::string src = configsource;
		(void)extractBooleanFromConfigString( useAcl, src, "acl", m_errorhnd);
		removeKeyFromConfigString( src, "statsproc", m_errorhnd);
		if (m_errorhnd->hasError()) return false;

		if (!dbi->createDatabase( src)) throw std::runtime_error( _TXT("failed to create key/value store database"));
		strus::local_ptr<strus::DatabaseClientInterface> database( dbi->createClient( src));
		if (!database.get()) throw std::runtime_error( _TXT("failed to create database client"));

		// 1st phase, store variables:
		strus::local_ptr<DatabaseTransactionInterface> transaction( database->createTransaction());
		if (!transaction.get()) return false;

		DatabaseAdapter_Variable<Index>::Writer stor( database.get());

		stor.store( transaction.get(), "TermNo", 1);
		stor.store( transaction.get(), "TypeNo", 1);
		stor.store( transaction.get(), "StructNo", 1);
		stor.store( transaction.get(), "DocNo", 1);
		stor.store( transaction.get(), "AttribNo", 1);
		stor.store( transaction.get(), "NofDocs", 0);
		stor.store( transaction.get(), "ByteOrderMark", byteOrderMark.value());
		stor.store( transaction.get(), "Version", (STRUS_STORAGE_VERSION_MAJOR * 1000) + STRUS_STORAGE_VERSION_MINOR);
		if (useAcl)
		{
			stor.store( transaction.get(), "UserNo", 1);
		}
		return transaction->commit();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage (physically): %s"), *m_errorhnd, false);
}

bool Storage::destroyStorage( const std::string& configsource, const DatabaseInterface* database) const
{
	try
	{
		std::string src = configsource;
		std::string path;
		if (!strus::extractStringFromConfigString( path, src, "path", m_errorhnd))
		{
			throw std::runtime_error(_TXT("no path specified"));
		}
		if (!database->destroyDatabase( configsource))
		{
			return false;
		}
		(void)strus::removeDirRecursive( path);
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error destroying storage: %s"), *m_errorhnd, false);
}

const char* Storage::getConfigDescription() const
{
	return _TXT("cachedterms=<file with list of terms to cache (only for client)>\nacl=<yes/no, yes if users with different access rights exist (only on creating the storage)>");
}

const char** Storage::getConfigParameters() const
{
	static const char* keys[] = {"cachedterms", "acl", 0};
	return keys;
}


