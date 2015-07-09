#include "database.hpp"
#include "databaseClient.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseBackupCursorInterface.hpp"
#include "strus/private/configParser.hpp"
#include "database.hpp"
#include "private/dll_tags.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>
#include <memory>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace strus;

DatabaseClientInterface* Database::createClient( const std::string& configsource) const
{
	unsigned int cachesize_kb = 0;
	bool compression = true;
	unsigned int maxOpenFiles = 0;
	unsigned int writeBufferSize = 0;
	unsigned int blockSize = 0;
	std::string path;
	std::string src( configsource);

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw strus::runtime_error( _TXT( "missing 'path' in database configuration string"));
	}
	(void)extractBooleanFromConfigString( compression, src, "compression");
	(void)extractUIntFromConfigString( cachesize_kb, src, "cache");
	(void)extractUIntFromConfigString( maxOpenFiles, src, "max_open_files");
	(void)extractUIntFromConfigString( writeBufferSize, src, "write_buffer_size");
	(void)extractUIntFromConfigString( blockSize, src, "block_size");

	return new DatabaseClient( m_dbhandle_map, path.c_str(), maxOpenFiles, cachesize_kb, compression, writeBufferSize, blockSize);
}

void Database::createDatabase( const std::string& configsource) const
{
	bool compression = true;
	std::string path;
	std::string src = configsource;

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw strus::runtime_error( _TXT( "missing 'path' in database configuration string"));
	}
	(void)extractBooleanFromConfigString( compression, src, "compression");

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
		throw strus::runtime_error( _TXT( "failed to create LevelDB key value store database: %s"), err.c_str());
	}
	if (db) delete db;
}

void Database::destroyDatabase( const std::string& configsource) const
{
	std::string path;
	std::string src = configsource;

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw strus::runtime_error( _TXT( "missing 'path' in database configuration string"));
	}

	leveldb::Options options;
	leveldb::Status status = leveldb::DestroyDB( path, options);
	if (!status.ok())
	{
		std::string err = status.ToString();
		throw strus::runtime_error( _TXT( "failed to remove key value store database: "), err.c_str());
	}
}

void Database::restoreDatabase( const std::string& configsource, DatabaseBackupCursorInterface* backup) const
{
	createDatabase( configsource);
	leveldb::DB* db = 0;
	std::auto_ptr<leveldb::DB> dbref;

	// Open the database created:
	std::string path;
	std::string src = configsource;

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw strus::runtime_error( _TXT( "missing 'path' in database configuration string"));
	}
	leveldb::Status status = leveldb::DB::Open( leveldb::Options(), path, &db);
	if (!status.ok())
	{
		std::string err = status.ToString();
		if (db) delete db;
		throw strus::runtime_error( _TXT( "failed to open LevelDB key value store database for restoring backup: %s"), err.c_str());
	}
	else
	{
		dbref.reset( db);
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
				throw strus::runtime_error( _TXT( "error in commit when writing backup restore batch: "), statusstr.c_str());
			}
			batch.Clear();
			blkcnt = 0;
		}
	}
	if (blkcnt > 0)
	{
		leveldb::Status status = db->Write( options, &batch);
		if (!status.ok())
		{
			std::string statusstr( status.ToString());
			throw strus::runtime_error( _TXT( "error in commit when writing backup restore batch: "), statusstr.c_str());
		}
		batch.Clear();
	}
}

const char* Database::getConfigDescription( ConfigType type) const
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

const char** Database::getConfigParameters( ConfigType type) const
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


