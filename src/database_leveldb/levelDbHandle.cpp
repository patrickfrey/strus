/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "levelDbHandle.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
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

void LevelDbConnection::close()
{
	if (m_db.get())
	{
		delete_iterators();

		// Dereference if this connection is the last one:
		const char* path = m_db->path().c_str();
		m_db.reset();
		m_dbmap->dereference( path);
	}
}

void LevelDbConnection::delete_iterators()
{
	utils::ScopedLock lock( m_mutex);
	std::list<leveldb::Iterator*>::iterator ii = m_itrlist.begin(), ie = m_itrlist.end();
	for (; ii != ie; ++ii)
	{
		if (*ii)
		{
			delete *ii;
			*ii = 0;
		}
	}
}

LevelDbConnection::IteratorHandle LevelDbConnection::newIterator( const leveldb::ReadOptions& opt)
{
	utils::ScopedLock lock( m_mutex);
	m_itrlist.push_back((leveldb::Iterator*)0);

	leveldb::DB* dbh = db();
	if (!dbh) throw strus::runtime_error(_TXT("cannot create a levelDB iterator on a connection closed"));
	leveldb::Iterator* itr = dbh->NewIterator( opt);
	if (!itr) throw strus::runtime_error(_TXT("failed to create a levelDB iterator"));

	m_itrlist.back() = itr;
	IteratorHandle last = m_itrlist.end();
	return --last;
}

void LevelDbConnection::deleteIterator( IteratorHandle& ihnd)
{
	if (*ihnd)
	{
		delete *ihnd;
	}
	utils::ScopedLock lock( m_mutex);
	m_itrlist.erase( ihnd);
}

