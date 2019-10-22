/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DATABASE_CLIENT_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_CLIENT_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseClientInterface.hpp"
#include "levelDbHandle.hpp"
#include "strus/base/shared_ptr.hpp"
#include <leveldb/db.h>
#include "strus/base/shared_ptr.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Implementation of the strus key value storage database based on the LevelDB library
class DatabaseClient
	:public DatabaseClientInterface
{
public:
	/// \brief Constructor
	/// \param[in] dbmap pointer to map of levelDB handles (singleton)
	/// \param[in] path path of the storage
	/// \param[in] maxOpenFiles maximum number of files open (0 for default)
	/// \param[in] cachesize_k number of K LRU cache for nodes
	/// \param[in] compression wheter to use snappy compression (true) or not
	/// \param[in] writeBufferSize size of write buffer per file
	/// \param[in] blockSize block size on disk (size of units)
	/// \param[in] autocompaction_ true if compaction is called on every transaction commit, false else
	DatabaseClient(
			LevelDbHandleMap* dbmap,
			const std::string& path,
			unsigned int maxOpenFiles,
			unsigned int cachesize_k,
			bool compression,
			unsigned int writeBufferSize,
			unsigned int blockSize,
			bool autocompaction_,
			ErrorBufferInterface* errorhnd_)
		:m_conn( new LevelDbConnection( dbmap, dbmap->create( path, maxOpenFiles, cachesize_k, compression, writeBufferSize, blockSize)))
		,m_autocompaction(autocompaction_)
		,m_errorhnd(errorhnd_)
	{}

	virtual ~DatabaseClient();

	virtual DatabaseTransactionInterface* createTransaction();

	virtual DatabaseCursorInterface* createCursor( const DatabaseOptions& options) const;

	virtual DatabaseBackupCursorInterface* createBackupCursor() const;
	
	virtual void writeImm(
			const char* key,
			std::size_t keysize,
			const char* value,
			std::size_t valuesize);

	virtual void removeImm(
			const char* key,
			std::size_t keysize);

	virtual bool readValue(
			const char* key,
			std::size_t keysize,
			std::string& value,
			const DatabaseOptions& options) const;

	virtual std::string config() const;

	virtual bool compactDatabase();

	virtual void close();

private:
	strus::shared_ptr<LevelDbConnection> m_conn;		///< reference to database connection
	bool m_autocompaction;					///< true if compaction is called on every transaction commit (configuration autocompact=yes)
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif


