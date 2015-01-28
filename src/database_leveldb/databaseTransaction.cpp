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
#include "databaseTransaction.hpp"
#include "databaseCursor.hpp"
#include "database.hpp"

using namespace strus;

DatabaseTransaction::DatabaseTransaction( leveldb::DB* db_, Database* database_)
	:m_database(database_),m_db(db_),m_commit_called(false),m_rollback_called(false)
{}

DatabaseTransaction::~DatabaseTransaction()
{
	if (!m_commit_called && !m_rollback_called) rollback();
}

DatabaseCursorInterface* DatabaseTransaction::createCursor( bool useCache) const
{
	return m_database->createCursor( useCache);
}

void DatabaseTransaction::write(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize)
{
	m_batch.Put(
		leveldb::Slice( key, keysize),
		leveldb::Slice( value, valuesize));
}

void DatabaseTransaction::remove(
			const char* key,
			std::size_t keysize)
{
	m_batch.Delete( leveldb::Slice( key, keysize));
}

void DatabaseTransaction::commit()
{
	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Write( options, &m_batch);
	if (!status.ok())
	{
		throw std::runtime_error( std::string( "error in commit when writing transaction batch: ") + status.ToString());
	}
	m_batch.Clear();
	m_database->releaseTransaction();
	m_commit_called = true;
}

void DatabaseTransaction::rollback()
{
	m_batch.Clear();
	m_database->releaseTransaction();
	m_rollback_called = true;
}

