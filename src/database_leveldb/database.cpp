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
#include "database.hpp"
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "strus/reference.hpp"
#include <stdexcept>
#include <leveldb/db.h>
#include <leveldb/cache.h>

using namespace strus;

Database::Database( const char* path_, unsigned int cachesize_k, bool compression)
	:m_db(0),m_closed(false),m_transactionOpen(0)
{
	m_dboptions.create_if_missing = false;
	if (cachesize_k)
	{
		if (cachesize_k * 1024 < cachesize_k) throw std::runtime_error("size of cache out of range");
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
		throw std::runtime_error( std::string( "failed to open key calue store database: ") + err);
	}
}

Database::~Database()
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

void Database::cleanup()
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

void Database::close()
{
	boost::mutex::scoped_lock lock( m_transactionOpen_mutex);
	if (m_transactionOpen)
	{
		throw std::logic_error( "tried to close key value store database with a transaction alive");
	}
}

DatabaseTransactionInterface* Database::createTransaction()
{
	boost::mutex::scoped_lock lock( m_transactionOpen_mutex);
	if (m_transactionOpen)
	{
		throw std::runtime_error( "this key value store database implementation allows only one transaction at a time");
	}
	DatabaseTransaction* rt = new DatabaseTransaction( m_db, this);
	m_transactionOpen = true;
	return rt;
}

void Database::releaseTransaction()
{
	boost::mutex::scoped_lock lock( m_transactionOpen_mutex);
	if (!m_transactionOpen)
	{
		throw std::logic_error( "something very bad happened: transaction flag inconsistency");
	}
	m_transactionOpen = false;
}

DatabaseCursorInterface* Database::createCursor( bool useCache)
{
	return new DatabaseCursor( m_db, useCache);
}

void Database::writeImm(
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
		throw std::runtime_error( status.ToString());
	}
}

void Database::removeImm(
			const char* key,
			std::size_t keysize)
{
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Delete( options, leveldb::Slice( key, keysize));
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
}

bool Database::readValue(
		const char* key,
		std::size_t keysize,
		std::string& value,
		bool useCache) const
{
	std::string rt;
	leveldb::ReadOptions options;
	options.fill_cache = useCache;

	leveldb::Status status = m_db->Get( options, leveldb::Slice( key, keysize), &value);
	if (status.IsNotFound())
	{
		return false;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	return true;
}

