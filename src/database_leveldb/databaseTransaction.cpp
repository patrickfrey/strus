/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "databaseClient.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/databaseOptions.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <memory>
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseTransaction::DatabaseTransaction( leveldb::DB* db_, DatabaseClient* database_, ErrorBufferInterface* errorhnd_)
	:m_database(database_),m_db(db_),m_commit_called(false),m_rollback_called(false),m_errorhnd(errorhnd_)
{}

DatabaseTransaction::~DatabaseTransaction()
{
	if (!m_commit_called && !m_rollback_called) rollback();
}

DatabaseCursorInterface* DatabaseTransaction::createCursor( const DatabaseOptions& options) const
{
	try
	{
		return m_database->createCursor( options);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database cursor: %s"), *m_errorhnd, 0);
}

void DatabaseTransaction::write(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	try
	{
		m_batch.Put(
			leveldb::Slice( key, keysize),
			leveldb::Slice( value, valuesize));
	}
	CATCH_ERROR_MAP( _TXT("error writing element in database transaction: %s"), *m_errorhnd);
}

void DatabaseTransaction::remove(
			const char* key,
			std::size_t keysize)
{
	try
	{
		m_batch.Delete( leveldb::Slice( key, keysize));
	}
	CATCH_ERROR_MAP( _TXT("error removing element in database transaction: %s"), *m_errorhnd);
}

void DatabaseTransaction::removeSubTree(
		const char* domainkey,
		std::size_t domainkeysize)
{
	try
	{
		std::auto_ptr<leveldb::Iterator> itr( m_db->NewIterator( leveldb::ReadOptions()));
		for (itr->Seek( leveldb::Slice( domainkey,domainkeysize));
			itr->Valid()
				&& domainkeysize <= itr->key().size()
				&& 0==std::memcmp( itr->key().data(), domainkey, domainkeysize);
			itr->Next())
		{
			m_batch.Delete( itr->key());
		}
	}
	CATCH_ERROR_MAP( _TXT("error removing subtree in database transaction: %s"), *m_errorhnd);
}

bool DatabaseTransaction::commit()
{
	try
	{
		if (m_errorhnd->hasError())
		{
			m_errorhnd->explain( _TXT( "database transaction with error: %s"));
			return false;
		}
		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->Write( options, &m_batch);
		if (!status.ok())
		{
			std::string statusstr( status.ToString());
			m_errorhnd->report( _TXT( "error in commit when writing transaction batch: "), statusstr.c_str());
			return false;
		}
		m_batch.Clear();
		m_commit_called = true;
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in database transaction commit: %s"), *m_errorhnd, false);
}

void DatabaseTransaction::rollback()
{
	m_batch.Clear();
	m_rollback_called = true;
}

