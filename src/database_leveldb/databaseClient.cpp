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
#include "getMemorySize.h"
#include <stdexcept>
#include <leveldb/db.h>
#include <leveldb/cache.h>

using namespace strus;

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

LevelDbHandle::LevelDbHandle( const std::string& path_, unsigned int maxOpenFiles_, unsigned int cachesize_k_, bool compression_, unsigned int writeBufferSize_, unsigned int blockSize_, bool createIfMissing_)
	:m_path(path_),m_db(0)
	,m_maxOpenFiles(maxOpenFiles_)
	,m_cachesize_k(cachesize_k_)
	,m_compression(compression_)
	,m_writeBufferSize(writeBufferSize_)
	,m_blockSize(blockSize_)

{
	m_dboptions.create_if_missing = createIfMissing_;
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
	if (!m_writeBufferSize)
	{
		m_dboptions.write_buffer_size = m_writeBufferSize;
	}
	if (!m_blockSize)
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

utils::SharedPtr<LevelDbHandle> LevelDbHandleMap::create( const std::string& path_, unsigned int maxOpenFiles_, unsigned int cachesize_k_, bool compression_, unsigned int writeBufferSize_, unsigned int blockSize_, bool createIfMissing_)
{
	utils::ScopedLock lock( m_map_mutex);
	std::string path = normalizePath( path_);
	std::map<std::string,utils::SharedPtr<LevelDbHandle> >::iterator mi = m_map.find( path);
	if (mi == m_map.end())
	{
		utils::SharedPtr<LevelDbHandle> rt( new LevelDbHandle( path_, maxOpenFiles_, cachesize_k_, compression_, writeBufferSize_, blockSize_, createIfMissing_));
		m_map[ path_] = rt;
		return rt;
	}
	else
	{
		if ((maxOpenFiles_ && mi->second->maxOpenFiles() != maxOpenFiles_)
		||  (cachesize_k_ && mi->second->cachesize_k() != cachesize_k_)
		||  (compression_ != mi->second->compression())
		||  (writeBufferSize_ && mi->second->writeBufferSize() != writeBufferSize_)
		||  (blockSize_ && mi->second->blockSize() != blockSize_))
		{
			throw strus::runtime_error( _TXT( "level DB key value store with the same path opened twice but with different settings"));
		}
		return mi->second;
	}
}

void LevelDbHandleMap::dereference( const std::string& path_)
{
	utils::ScopedLock lock( m_map_mutex);
	std::map<std::string,utils::SharedPtr<LevelDbHandle> >::iterator mi = m_map.find( path_);
	if (mi != m_map.end())
	{
		if (mi->second.unique())
		{
			m_map.erase( path_);
		}
	}
}

LevelDbHandle::~LevelDbHandle()
{
	cleanup();
}

DatabaseTransactionInterface* DatabaseClient::createTransaction()
{
	return new DatabaseTransaction( m_db->db(), this);
}

DatabaseCursorInterface* DatabaseClient::createCursor( const DatabaseOptions& options) const
{
	return new DatabaseCursor( m_db->db(), options.useCacheEnabled());
}


class DatabaseBackupCursor
	:public DatabaseBackupCursorInterface
	,public DatabaseCursor
{
public:
	explicit DatabaseBackupCursor( leveldb::DB* db_)
		:DatabaseCursor( db_, false, true){}

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
	return new DatabaseBackupCursor( m_db->db());
}

void DatabaseClient::writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->db()->Put( options,
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
	leveldb::Status status = m_db->db()->Delete( options, leveldb::Slice( key, keysize));
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

	leveldb::Status status = m_db->db()->Get( readoptions, leveldb::Slice( key, keysize), &value);
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

