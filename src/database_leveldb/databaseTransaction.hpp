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
#ifndef _STRUS_DATABASE_TRANSACTION_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_TRANSACTION_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseTransactionInterface.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus
{

/// \brief Forward declaration
class Database;

/// \brief Implementation of DatabaseTransactionInterface based on the LevelDB library
class DatabaseTransaction
	:public DatabaseTransactionInterface
{
public:
	DatabaseTransaction( leveldb::DB* db_, Database* database_);

	virtual ~DatabaseTransaction();

	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const;

	virtual void write(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize);

	virtual void remove(
			const char* key,
			std::size_t keysize);

	virtual void removeSubTree(
			const char* domainkey,
			std::size_t domainkeysize);

	virtual void commit();

	virtual void rollback();

private:
	Database* m_database;
	leveldb::DB* m_db;			///< levelDB handle
	leveldb::WriteBatch m_batch;		///< batch used for the transaction
	bool m_commit_called;			///< true, if the transaction has been committed
	bool m_rollback_called;			///< true, if the transaction has been rolled back
};

}//namespace
#endif


