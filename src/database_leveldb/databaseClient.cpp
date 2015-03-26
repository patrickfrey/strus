/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "databaseClient.hpp"
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "strus/databaseBackupCursorInterface.hpp"
#include "strus/reference.hpp"
#include "strus/databaseOptions.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>
#include <leveldb/db.h>
#include <leveldb/cache.h>

using namespace strus;

DatabaseClient::DatabaseClient( const char* path_, unsigned int cachesize_k, bool compression)
	:m_db(0),m_closed(false)
{
	m_dboptions.create_if_missing = false;
	if (cachesize_k)
	{
		if (cachesize_k * 1024 < cachesize_k) throw strus::runtime_error( _TXT( "size of cache out of range"));
		m_dboptions.block_cache = leveldb::NewLRUCache( cachesize_k * 1024);
	}
	if (!compression)
	{
		//... compression reduces size of index by 25% and has about 10% better performance
		m_dboptions.compression = leveldb::kNoCompression;
	}
	leveldb::Status status = leveldb::DB::Open( m_dboptions, path_, &m_db);
	if (!status.ok())
	{
		std::string err = status.ToString();
		cleanup();
		throw strus::runtime_error( _TXT( "failed to open key calue store database: %s"), err.c_str());
	}
}

DatabaseClient::~DatabaseClient()
{
	try
	{
		if (!m_closed) close();
	}
	catch (const std::runtime_error&)
	{
		/// ... silently ignore close errors. Call close directly to catch them
	}
	cleanup();
}

void DatabaseClient::cleanup()
{
	if (m_db)
	{
		delete m_db;
		m_db = 0;
	}
	if (m_dboptions.block_cache)
	{
		delete m_dboptions.block_cache;
		m_dboptions.block_cache = 0;
	}
}

void DatabaseClient::close()
{
}

DatabaseTransactionInterface* DatabaseClient::createTransaction()
{
	return new DatabaseTransaction( m_db, this);
}

DatabaseCursorInterface* DatabaseClient::createCursor( const DatabaseOptions& options) const
{
	return new DatabaseCursor( m_db, options.useCacheEnabled());
}


class DatabaseBackupCursor
	:public DatabaseBackupCursorInterface
	,public DatabaseCursor
{
public:
	explicit DatabaseBackupCursor( leveldb::DB* db_)
		:DatabaseCursor( db_, false){}

	virtual bool fetch(
			const char*& key,
			std::size_t& keysize,
			const char*& blk,
			std::size_t& blksize)
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
private:
	Slice m_key;
};


DatabaseBackupCursorInterface* DatabaseClient::createBackupCursor() const
{
	return new DatabaseBackupCursor( m_db);
}

void DatabaseClient::writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Put( options,
					leveldb::Slice( key, keysize),
					leveldb::Slice( value, valuesize));
	if (!status.ok())
	{
		std::string ststr( status.ToString());
		throw strus::runtime_error( _TXT( "Level DB error: %s"), ststr.c_str());
	}
}

void DatabaseClient::removeImm(
			const char* key,
			std::size_t keysize)
{
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Delete( options, leveldb::Slice( key, keysize));
	if (!status.ok())
	{
		std::string ststr( status.ToString());
		throw strus::runtime_error( _TXT( "Level DB error: %s"), ststr.c_str());
	}
}

bool DatabaseClient::readValue(
		const char* key,
		std::size_t keysize,
		std::string& value,
		const DatabaseOptions& options) const
{
	std::string rt;
	leveldb::ReadOptions readoptions;
	readoptions.fill_cache = options.useCacheEnabled();

	leveldb::Status status = m_db->Get( readoptions, leveldb::Slice( key, keysize), &value);
	if (status.IsNotFound())
	{
		return false;
	}
	if (!status.ok())
	{
		std::string ststr( status.ToString());
		throw strus::runtime_error( _TXT( "Level DB error: %s"), ststr.c_str());
	}
	return true;
}

