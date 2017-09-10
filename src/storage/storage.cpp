/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storage.hpp"
#include "storageClient.hpp"
#include "storageDump.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDumpInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/versionStorage.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "storageAlterMetaDataTable.hpp"
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

static std::string loadFile( const std::string& filename)
{
	std::string rt;
	int errcode = 0;

	FILE* fh = ::fopen( filename.c_str(), "rb");
	if (!fh)
	{
		errcode = errno;
		goto FILEERROR;
	}
	unsigned int nn;
	enum {bufsize=(1<<12)};
	char buf[ bufsize];

	while (!!(nn=::fread( buf, 1, bufsize, fh)))
	{
		try
		{
			rt.append( buf, nn);
		}
		catch (const std::bad_alloc&)
		{
			errcode = 12/*ENOMEM*/;
			goto FILEERROR;
		}
	}
	if (!feof( fh))
	{
		errcode = ::ferror( fh);
		goto FILEERROR;
	}
	else
	{
		::fclose( fh);
	}
	return rt;
FILEERROR:
	{
		if (fh) ::fclose( fh);
		throw strus::runtime_error( _TXT( "could not read file '%s': (errno %d)"), filename.c_str(), errcode);
	}
}


StorageClientInterface* Storage::createClient(
		const std::string& configsource,
		const DatabaseInterface* database,
		const StatisticsProcessorInterface* statisticsProc) const
{
	try
	{
		std::string cachedterms;
		std::string databaseConfig = configsource;
		if (extractStringFromConfigString( cachedterms, databaseConfig, "cachedterms", m_errorhnd))
		{
			std::string cachedtermsrc = loadFile( cachedterms);
			return new StorageClient( database, databaseConfig, cachedtermsrc.c_str(), statisticsProc, m_errorhnd);
		}
		else
		{
			if (m_errorhnd->hasError())
			{
				m_errorhnd->explain(_TXT("error creating storage client: %s"));
				return 0;
			}
			return new StorageClient( database, databaseConfig, 0, statisticsProc, m_errorhnd);
		}
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
		std::string metadata;
		ByteOrderMark byteOrderMark;

		std::string src = configsource;
		(void)extractStringFromConfigString( metadata, src, "metadata", m_errorhnd);
		(void)extractBooleanFromConfigString( useAcl, src, "acl", m_errorhnd);
		if (m_errorhnd->hasError()) return false;

		if (!dbi->createDatabase( src)) throw strus::runtime_error(_TXT("failed to create key/value store database"));
		strus::local_ptr<strus::DatabaseClientInterface> database( dbi->createClient( src));
		if (!database.get()) throw strus::runtime_error(_TXT("failed to create database client"));

		// 1st phase, store variables:
		strus::local_ptr<DatabaseTransactionInterface> transaction( database->createTransaction());
		if (!transaction.get()) return false;

		DatabaseAdapter_Variable::Writer stor( database.get());
	
		stor.store( transaction.get(), "TermNo", 1);
		stor.store( transaction.get(), "TypeNo", 1);
		stor.store( transaction.get(), "DocNo", 1);
		stor.store( transaction.get(), "AttribNo", 1);
		stor.store( transaction.get(), "NofDocs", 0);
		stor.store( transaction.get(), "ByteOrderMark", byteOrderMark.value());
		stor.store( transaction.get(), "Version", (STRUS_STORAGE_VERSION_MAJOR * 1000) + STRUS_STORAGE_VERSION_MINOR);
		if (useAcl)
		{
			stor.store( transaction.get(), "UserNo", 1);
		}
		if (!transaction->commit()) return false;

		// 2nd phase, store metadata:
		transaction.reset( database->createTransaction());
		if (!transaction.get()) return false;

		MetaDataDescription md( metadata);
		md.store( transaction.get());
	
		return transaction->commit();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *m_errorhnd, false);
}

StorageAlterMetaDataTableInterface* Storage::createAlterMetaDataTable(
		const std::string& configsource,
		const DatabaseInterface* database) const
{
	try
	{
		return new StorageAlterMetaDataTable( database, configsource, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage client: %s"), *m_errorhnd, 0);
}

const char* Storage::getConfigDescription( const ConfigType& type) const
{
	switch (type)
	{
		case CmdCreateClient:
			return "cachedterms=<file with list of terms to cache>";

		case CmdCreate:
			return "acl=<yes/no, yes if users with different access rights exist>\nmetadata=<comma separated list of meta data def>";
	}
	return 0;
}

const char** Storage::getConfigParameters( const ConfigType& type) const
{
	static const char* keys_CreateStorageClient[]	= {"cachedterms", 0};
	static const char* keys_CreateStorage[]		= {"acl", "metadata", 0};
	switch (type)
	{
		case CmdCreateClient:	return keys_CreateStorageClient;
		case CmdCreate:		return keys_CreateStorage;
	}
	return 0;
}

StorageDumpInterface* Storage::createDump(
		const std::string& configsource,
		const DatabaseInterface* database,
		const std::string& keyprefix) const
{
	try
	{
		return new StorageDump( database, configsource, keyprefix, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating storage dump interface: %s"), *m_errorhnd, 0);
}


