/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DATABASE_TRANSACTION_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_TRANSACTION_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseTransactionInterface.hpp"
#include "levelDbHandle.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Forward declaration
class DatabaseClient;

/// \brief Implementation of DatabaseTransactionInterface based on the LevelDB library
class DatabaseTransaction
	:public DatabaseTransactionInterface
{
public:
	DatabaseTransaction( const utils::SharedPtr<LevelDbConnection>& conn_, ErrorBufferInterface* errorhnd_);

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

	virtual bool commit();

	virtual void rollback();

private:
	utils::SharedPtr<LevelDbConnection> m_conn;	///< levelDB connection
	leveldb::WriteBatch m_batch;			///< batch used for the transaction
	bool m_commit_called;				///< true, if the transaction has been committed
	bool m_rollback_called;				///< true, if the transaction has been rolled back
	ErrorBufferInterface* m_errorhnd;		///< buffer for reporting errors
};

}//namespace
#endif


