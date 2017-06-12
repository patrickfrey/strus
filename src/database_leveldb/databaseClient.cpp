/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "databaseClient.hpp"
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "strus/databaseBackupCursorInterface.hpp"
#include "strus/reference.hpp"
#include "strus/databaseOptions.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "getMemorySize.h"
#include <iostream>
#include <sstream>
#include <leveldb/db.h>
#include <leveldb/cache.h>

using namespace strus;

#define MODULENAME "databaseClient"

DatabaseClient::~DatabaseClient()
{
	try
	{
		if (m_conn->db())
		{
			m_conn->close();
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in destructor of '%s': %s"), MODULENAME, *m_errorhnd);
}

DatabaseTransactionInterface* DatabaseClient::createTransaction()
{
	try
	{
		if (!m_conn->db()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createTransaction");
		return new DatabaseTransaction( m_conn, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating transaction: %s"), *m_errorhnd, 0);
}

DatabaseCursorInterface* DatabaseClient::createCursor( const DatabaseOptions& options) const
{
	try
	{
		if (!m_conn->db()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createCursor");
		return new DatabaseCursor( m_conn, options.useCacheEnabled(), false, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database cursor: %s"), *m_errorhnd, 0);
}


class DatabaseBackupCursor
	:public DatabaseBackupCursorInterface
	,public DatabaseCursor
{
public:
	DatabaseBackupCursor( const utils::SharedPtr<LevelDbConnection>& conn_, ErrorBufferInterface* errorhnd_)
		:DatabaseCursor( conn_, false, true, errorhnd_),m_errorhnd(errorhnd_){}

	virtual bool fetch(
			const char*& key,
			std::size_t& keysize,
			const char*& blk,
			std::size_t& blksize)
	{
		try
		{
			if (!m_key.defined())
			{
				m_key = seekFirst( 0, 0);
			}
			else
			{
				m_key = seekNext();
			}
			if (!m_key.defined()) return false;
			Slice blkslice = value();
			key = m_key.ptr();
			keysize = m_key.size();
			blk = blkslice.ptr();
			blksize = blkslice.size();
			return true;
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error in database cursor fetching next element: %s"), *m_errorhnd, false);
	}

private:
	Slice m_key;
	ErrorBufferInterface* m_errorhnd;
};


DatabaseBackupCursorInterface* DatabaseClient::createBackupCursor() const
{
	try
	{
		if (!m_conn->db()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createBackupCursor");
		return new DatabaseBackupCursor( m_conn, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' backup cursor: %s"), MODULENAME, *m_errorhnd, 0);
}

void DatabaseClient::writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	try
	{
		leveldb::DB* db = m_conn->db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "writeImm");

		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = db->Put( options,
						leveldb::Slice( key, keysize),
						leveldb::Slice( value, valuesize));
		if (!status.ok())
		{
			std::string ststr( status.ToString());
			m_errorhnd->report( _TXT( "leveldb error: %s"), ststr.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' writeImm: %s"), MODULENAME, *m_errorhnd);
}

void DatabaseClient::removeImm(
			const char* key,
			std::size_t keysize)
{
	try
	{
		leveldb::DB* db = m_conn->db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "removeImm");

		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = db->Delete( options, leveldb::Slice( key, keysize));
		if (!status.ok())
		{
			std::string ststr( status.ToString());
			m_errorhnd->report( _TXT( "leveldb error: %s"), ststr.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' removeImm: %s"), MODULENAME, *m_errorhnd);
}

bool DatabaseClient::readValue(
		const char* key,
		std::size_t keysize,
		std::string& value,
		const DatabaseOptions& options) const
{
	try
	{
		leveldb::DB* db = m_conn->db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "readValue");

		std::string rt;
		leveldb::ReadOptions readoptions;
		readoptions.fill_cache = options.useCacheEnabled();

		leveldb::Status status = db->Get( readoptions, leveldb::Slice( key, keysize), &value);
		if (status.IsNotFound())
		{
			return false;
		}
		if (!status.ok())
		{
			std::string ststr( status.ToString());
			m_errorhnd->report( _TXT( "leveldb error: %s"), ststr.c_str());
		}
		return true;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error '%s' readValue: %s"), MODULENAME, *m_errorhnd, false);
}

void DatabaseClient::close()
{
	try
	{
		leveldb::DB* db = m_conn->db();
		if (!db) return;

		// Do explicit compaction:
		db->CompactRange( NULL, NULL);
		m_conn->close();
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' close: %s"), MODULENAME, *m_errorhnd);
}

std::string DatabaseClient::config() const
{
	try
	{
		leveldb::DB* db = m_conn->db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "config");

		return m_conn->config();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in '%s' mapping configuration to string: %s"), MODULENAME, *m_errorhnd, std::string());
}


