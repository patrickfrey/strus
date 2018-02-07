/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "database.hpp"
#include "databaseClient.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseBackupCursorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/configParser.hpp"
#include "strus/base/fileio.hpp"
#include "database.hpp"
#include "leveldbErrorCode.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <cstring>
#include <memory>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "strus/base/fileio.hpp"
#include "strus/base/local_ptr.hpp"

using namespace strus;

DatabaseClientInterface* Database::createClient( const std::string& configsource) const
{
	try
	{
		unsigned int cachesize = 0;
		unsigned int cachesize_kb = 0;
		bool compression = true;
		unsigned int maxOpenFiles = 0;
		unsigned int writeBufferSize = 0;
		unsigned int blockSize = 0;
		std::string path;
		std::string src( configsource);

		if (!extractStringFromConfigString( path, src, "path", m_errorhnd))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseIncompleteRequest), _TXT( "missing '%s' in database configuration string"), "path");
			return 0;
		}
		if (!expandDatabaseFullPath( path)) return NULL;

		if (!isDir( path))
		{
			throw strus::runtime_error( _TXT( "unknown path '%s' specified in database configuration string"), path.c_str());
		}
		(void)extractBooleanFromConfigString( compression, src, "compression", m_errorhnd);
		(void)extractUIntFromConfigString( cachesize, src, "cache", m_errorhnd);
		cachesize_kb = (unsigned int)((cachesize + 1023)/1024);
		(void)extractUIntFromConfigString( maxOpenFiles, src, "max_open_files", m_errorhnd);
		(void)extractUIntFromConfigString( writeBufferSize, src, "write_buffer_size", m_errorhnd);
		(void)extractUIntFromConfigString( blockSize, src, "block_size", m_errorhnd);
		if (m_errorhnd->hasError()) return 0;
		return new DatabaseClient( m_dbhandle_map, path.c_str(), maxOpenFiles, cachesize_kb, compression, writeBufferSize, blockSize, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database client: %s"), *m_errorhnd, 0);
}

bool Database::exists( const std::string& configsource) const
{
	try
	{
		std::string src = configsource;
		std::string path;

		if (!extractStringFromConfigString( path, src, "path", m_errorhnd))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseIncompleteRequest), _TXT( "missing '%s' in database configuration string"), "path");
			return false;
		}
		path.push_back( dirSeparator());
		path.append( "CURRENT");

		// ... this is a little bit a hack but levelDB version <= 1.15 always creates files
		// and this is not intended by a simple check
		return isFile( path);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error checking if database exists: %s"), *m_errorhnd, false);
}

bool Database::expandDatabaseFullPath( std::string& path) const
{
	if (!m_workdir.empty())
	{
		if (strus::hasUpdirReference( path))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseInvalidArgument), _TXT( "path in database configuration must not contain up-directory references ('..') if workdir is specified"));
			return false;
		}
		path = strus::joinFilePath( m_workdir, path);
	}
	return true;
}

bool Database::createDatabase( const std::string& configsource) const
{
	try
	{
		bool compression = true;
		std::string path;
		std::string src = configsource;

		if (!extractStringFromConfigString( path, src, "path", m_errorhnd))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseIncompleteRequest), _TXT( "missing '%s' in database configuration string"), "path");
			return false;
		}
		if (!expandDatabaseFullPath( path)) return false;
		(void)extractBooleanFromConfigString( compression, src, "compression", m_errorhnd);
		if (m_errorhnd->hasError()) return false;

		leveldb::DB* db = 0;
		leveldb::Options options;
		// Compression reduces size of index by 25% and has about 10% better performance
		// m_dboptions.compression = leveldb::kNoCompression;
		options.create_if_missing = true;
		options.error_if_exists = true;

		if (!compression)
		{
			options.compression = leveldb::kNoCompression;
		}
		leveldb::Status status = leveldb::DB::Open( options, path, &db);
		if (!status.ok())
		{
			std::string err = status.ToString();
			if (db) delete db;
			m_errorhnd->report( *leveldbErrorCode(ErrorOperationCreateStorage,status), _TXT( "failed to create LevelDB key value store database: %s"), err.c_str());
			return false;
		}
		if (db) delete db;
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating database: %s"), *m_errorhnd, false);
}

bool Database::destroyDatabase( const std::string& configsource) const
{
	try
	{
		std::string path;
		std::string src = configsource;

		if (!extractStringFromConfigString( path, src, "path", m_errorhnd))
		{
			m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseIncompleteRequest), _TXT( "missing '%s' in database configuration string"), "path");
			return false;
		}
		if (!expandDatabaseFullPath( path)) return false;

		leveldb::Options options;
		leveldb::Status status = leveldb::DestroyDB( path, options);
		if (!status.ok())
		{
			std::string err = status.ToString();
			m_errorhnd->report( *leveldbErrorCode(ErrorOperationDestroyStorage,status), _TXT( "failed to remove key value store database: %s"), err.c_str());
			return false;
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error destroying database: %s"), *m_errorhnd, false);
}

bool Database::restoreDatabase( const std::string& configsource, DatabaseBackupCursorInterface* backup) const
{
	if (!createDatabase( configsource)) return false;
	try
	{
		leveldb::DB* db = 0;
		strus::local_ptr<leveldb::DB> dbref;
		{
			// Open the database created:
			std::string path;
			std::string src = configsource;
	
			if (!extractStringFromConfigString( path, src, "path", m_errorhnd))
			{
				m_errorhnd->report( *ErrorCode(StrusComponentCore,ErrorOperationParse,ErrorCauseIncompleteRequest), _TXT( "missing '%s' in database configuration string"), "path");
				return false;
			}
			if (!expandDatabaseFullPath( path)) return false;

			leveldb::Status status = leveldb::DB::Open( leveldb::Options(), path, &db);
			if (!status.ok())
			{
				std::string err = status.ToString();
				if (db) delete db;
				m_errorhnd->report( *leveldbErrorCode(ErrorOperationOpenStorage,status), _TXT( "failed to open LevelDB key value store database for restoring backup: %s"), err.c_str());
				return false;
			}
			else
			{
				dbref.reset( db);
			}
		}
		unsigned int blkcnt = 0;
		leveldb::WriteBatch batch;
		leveldb::WriteOptions options;

		const char* key;
		std::size_t keysize;
		const char* blk;
		std::size_t blksize;

		// Restore backup loop:
		while (backup->fetch( key, keysize, blk, blksize))
		{
			batch.Put( leveldb::Slice( key, keysize), leveldb::Slice( blk, blksize));
			if (++blkcnt >= 1000)
			{
				leveldb::Status status = db->Write( options, &batch);
				if (!status.ok())
				{
					std::string statusstr( status.ToString());
					m_errorhnd->report( *leveldbErrorCode(ErrorOperationCommitStorage,status), _TXT( "error in commit when writing backup restore batch: %s"), statusstr.c_str());
					batch.Clear();
					return false;
				}
				batch.Clear();
				blkcnt = 0;
			}
		}
		if (m_errorhnd->hasError())
		{
			batch.Clear();
			return false;
		}
		if (blkcnt > 0)
		{
			leveldb::Status status = db->Write( options, &batch);
			if (!status.ok())
			{
				std::string statusstr( status.ToString());
				m_errorhnd->report( *leveldbErrorCode(ErrorOperationCommitStorage,status), _TXT( "error in commit when writing backup restore batch: %s"), statusstr.c_str());
				batch.Clear();
				return false;
			}
			batch.Clear();
		}
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error destroying database: %s"), *m_errorhnd, false);
}

const char* Database::getConfigDescription( const ConfigType& type) const
{
	switch (type)
	{
		case CmdCreateClient:
			return "path=<LevelDB storage path>\ncreate=<yes/no, yes=do create if database does not exist yet>\ncache=<size of LRU cache for LevelDB>\ncompression=<yes/no>\nmax_open_files=<maximum number of open files for LevelDB>\nwrite_buffer_size=<Amount of data to build up in memory per file>\nblock_size=<approximate size of user data packed per block>";

		case CmdCreate:
			return "path=<LevelDB storage path>;compression=<yes/no>";

		case CmdDestroy:
			return "path=<LevelDB storage path>";
	}
	return 0;
}

const char** Database::getConfigParameters( const ConfigType& type) const
{
	static const char* keys_CreateDatabaseClient[] = {"path","cache","compression","max_open_files","write_buffer_size","block_size",0};
	static const char* keys_CreateDatabase[] = {"path","compression", 0};
	static const char* keys_DestroyDatabase[] = {"path", 0};
	switch (type)
	{
		case CmdCreateClient:	return keys_CreateDatabaseClient;
		case CmdCreate:		return keys_CreateDatabase;
		case CmdDestroy:	return keys_DestroyDatabase;
	}
	return 0;
}

