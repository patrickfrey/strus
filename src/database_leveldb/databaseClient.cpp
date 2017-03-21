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

static std::string normalizePath( const std::string& path_)
{
	std::string rt;
	std::string::const_iterator pi = path_.begin(), pe = path_.end();
	for (; pi != pe; ++pi)
	{
		if (*pi == '/' || *pi == '\\')
		{
			while (pi+1 != pe && (pi[1] == '/' || pi[1] == '\\')) ++pi;
			if (pi != pe)
			{
				rt.push_back( '/');
			}
		}
		else
		{
			rt.push_back( *pi);
		}
	}
	return rt;
}

LevelDbHandle::LevelDbHandle( const std::string& path_, unsigned int maxOpenFiles_, unsigned int cachesize_k_, bool compression_, unsigned int writeBufferSize_, unsigned int blockSize_)
	:m_path(path_),m_db(0)
	,m_maxOpenFiles(maxOpenFiles_)
	,m_cachesize_k(cachesize_k_)
	,m_compression(compression_)
	,m_writeBufferSize(writeBufferSize_)
	,m_blockSize(blockSize_)

{
	m_dboptions.create_if_missing = false;
	if (m_maxOpenFiles)
	{
		m_dboptions.max_open_files = maxOpenFiles_;
	}
	if (m_cachesize_k)
	{
		if (m_cachesize_k * 1024 < m_cachesize_k) throw strus::runtime_error( _TXT( "size of cache out of range"));
		m_dboptions.block_cache = leveldb::NewLRUCache( m_cachesize_k * 1024);
	}
	if (!m_compression)
	{
		//... compression reduces size of index by 25% and has about 10% better performance
		m_dboptions.compression = leveldb::kNoCompression;
	}
	if (m_writeBufferSize)
	{
		m_dboptions.write_buffer_size = m_writeBufferSize;
	}
	if (m_blockSize)
	{
		m_dboptions.block_size = m_blockSize;
	}
	leveldb::Status status = leveldb::DB::Open( m_dboptions, path_.c_str(), &m_db);
	if (!status.ok())
	{
		std::string err = status.ToString();
		cleanup();
		throw strus::runtime_error( _TXT( "failed to open key value store database: %s"), err.c_str());
	}
	// Do compaction, if state of db was closed previously without:
	m_db->CompactRange( NULL, NULL);
}

std::string LevelDbHandle::config() const
{
	std::ostringstream out;
	out << "path='" << m_path << "'";
	if (!m_compression) out << ";compression=" << (m_compression?"Y":"N");
	if (m_cachesize_k) out << ";cache=" << m_cachesize_k << "K";
	if (m_maxOpenFiles) out << ";max_open_files=" << m_maxOpenFiles;
	if (m_writeBufferSize) out << ";write_buffer_size=" << m_writeBufferSize;
	if (m_blockSize) out << ";block_size=" << m_blockSize;
	return out.str();
}

void LevelDbHandle::cleanup()
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

utils::SharedPtr<LevelDbHandle> LevelDbHandleMap::create( const std::string& path_, unsigned int maxOpenFiles_, unsigned int cachesize_k_, bool compression_, unsigned int writeBufferSize_, unsigned int blockSize_)
{
	utils::ScopedLock lock( m_map_mutex);
	std::string path = normalizePath( path_);

	std::vector<LevelDbHandleRef>::iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if ((*mi)->path() == path_)
		{
			break;
		}
	}
	if (mi == m_map.end())
	{
		utils::SharedPtr<LevelDbHandle> rt( new LevelDbHandle( path_, maxOpenFiles_, cachesize_k_, compression_, writeBufferSize_, blockSize_));
		m_map.push_back( rt);
		return rt;
	}
	else
	{
		if ((maxOpenFiles_ && (*mi)->maxOpenFiles() != maxOpenFiles_)
		||  (cachesize_k_ && (*mi)->cachesize_k() != cachesize_k_)
		||  (compression_ != (*mi)->compression())
		||  (writeBufferSize_ && (*mi)->writeBufferSize() != writeBufferSize_)
		||  (blockSize_ && (*mi)->blockSize() != blockSize_))
		{
			throw strus::runtime_error( _TXT( "level DB key value store with the same path opened twice but with different settings"));
		}
		return *mi;
	}
}

void LevelDbHandleMap::dereference( const char* path)
{
	utils::ScopedLock lock( m_map_mutex);
	std::vector<LevelDbHandleRef>::iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (0==std::strcmp( (*mi)->path().c_str(), path))
		{
			if (mi->unique())
			{
				m_map.erase( mi);
			}
			break;
		}
	}
}

LevelDbHandle::~LevelDbHandle()
{
	cleanup();
}

DatabaseClient::~DatabaseClient()
{
	try
	{
		if (m_db.get())
		{
			// Dereference if this connection is the last one:
			const char* path = m_db->path().c_str();
			m_db.reset();
			m_dbmap->dereference( path);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in destructor of '%s': %s"), MODULENAME, *m_errorhnd);
}

DatabaseTransactionInterface* DatabaseClient::createTransaction()
{
	try
	{
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createTransaction");
		return new DatabaseTransaction( m_db->db(), this, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating transaction: %s"), *m_errorhnd, 0);
}

DatabaseCursorInterface* DatabaseClient::createCursor( const DatabaseOptions& options) const
{
	try
	{
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createCursor");
		return new DatabaseCursor( m_db->db(), options.useCacheEnabled(), false, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database cursor: %s"), *m_errorhnd, 0);
}


class DatabaseBackupCursor
	:public DatabaseBackupCursorInterface
	,public DatabaseCursor
{
public:
	DatabaseBackupCursor( leveldb::DB* db_, ErrorBufferInterface* errorhnd_)
		:DatabaseCursor( db_, false, true, errorhnd_),m_errorhnd(errorhnd_){}

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
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "createBackupCursor");
		return new DatabaseBackupCursor( m_db->db(), m_errorhnd);
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
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "writeImm");

		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->db()->Put( options,
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
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "removeImm");

		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->db()->Delete( options, leveldb::Slice( key, keysize));
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
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "readValue");

		std::string rt;
		leveldb::ReadOptions readoptions;
		readoptions.fill_cache = options.useCacheEnabled();

		leveldb::Status status = m_db->db()->Get( readoptions, leveldb::Slice( key, keysize), &value);
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
		if (!m_db.get()) return;

		// Do explicit compaction:
		m_db->db()->CompactRange( NULL, NULL);

		// Dereference if this connection is the last one:
		const char* path = m_db->path().c_str();
		m_db.reset();
		m_dbmap->dereference( path);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' close: %s"), MODULENAME, *m_errorhnd);
}

std::string DatabaseClient::config() const
{
	try
	{
		if (!m_db.get()) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "config");
		return m_db->config();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in '%s' mapping configuration to string: %s"), MODULENAME, *m_errorhnd, std::string());
}


