#include "database.hpp"
#include "databaseClient.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/private/configParser.hpp"
#include "database.hpp"
#include "private/dll_tags.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>
#include <leveldb/db.h>

using namespace strus;

DatabaseClientInterface* Database::createClient( const std::string& configsource) const
{
	unsigned int cachesize_kb = 0;
	bool compression = true;
	std::string path;
	std::string src( configsource);

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw strus::runtime_error( _TXT( "missing 'path' in database configuration string"));
	}
	(void)extractBooleanFromConfigString( compression, src, "compression");
	(void)extractUIntFromConfigString( cachesize_kb, src, "cache");

	return new DatabaseClient( path.c_str(), cachesize_kb, compression);
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

const char* Database::getConfigDescription( ConfigType type) const
{
	switch (type)
	{
		case CmdCreateClient:
			return "path=<LevelDB storage path>\ncache=<size of LRU cache for LevelDB>\ncompression=<yes/no>";

		case CmdCreate:
			return "path=<LevelDB storage path>;compression=<yes/no>";

		case CmdDestroy:
			return "path=<LevelDB storage path>";
	}
	return 0;
}

const char** Database::getConfigParameters( ConfigType type) const
{
	static const char* keys_CreateDatabaseClient[] = {"path","cache","compression", 0};
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


