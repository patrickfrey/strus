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
#include "strus/databaseLib.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/private/configParser.hpp"
#include "database.hpp"
#include "dll_tags.hpp"
#include <leveldb/db.h>

using namespace strus;

DLL_PUBLIC DatabaseInterface* strus::createDatabaseClient( const char* configsource)
{
	unsigned int cachesize_kb = 0;
	bool compression = true;
	std::string path;
	std::string src( configsource?configsource:"");

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw std::runtime_error("missing 'path' in database configuration string");
	}
	(void)extractBooleanFromConfigString( compression, src, "compression");
	(void)extractUIntFromConfigString( cachesize_kb, src, "cache");

	return new Database( path.c_str(), cachesize_kb, compression);
}


DLL_PUBLIC void strus::createDatabase( const char* configsource)
{
	bool compression = true;
	std::string path;
	std::string src( configsource?configsource:"");

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw std::runtime_error("missing 'path' in database configuration string");
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
	if (status.ok())
	{
		std::string err = status.ToString();
		if (db) delete db;
		throw std::runtime_error( std::string( "failed to create LevelDB key value store database: ") + err);
	}
	if (db) delete db;
}


DLL_PUBLIC void strus::destroyDatabase( const char* configsource)
{
	std::string path;
	std::string src( configsource?configsource:"");

	if (!extractStringFromConfigString( path, src, "path"))
	{
		throw std::runtime_error("missing 'path' in database configuration string");
	}

	leveldb::Options options;
	leveldb::Status status = leveldb::DestroyDB( path, options);
	if (!status.ok())
	{
		std::string err = status.ToString();
		throw std::runtime_error( std::string( "failed to remove key value store database: ") + err);
	}
}


DLL_PUBLIC const char* strus::getDatabaseConfigDescription( DatabaseConfigDescriptionType type)
{
	switch (type)
	{
		case CmdCreateDatabaseClient:
			return "path=<LevelDB storage path>\ncache=<size of LRU cache for LevelDB>\ncompression=<yes/no>";

		case CmdCreateDatabase:
			return "path=<LevelDB storage path>;compression=<yes/no>";

		case CmdDestroyDatabase:
			return "path=<LevelDB storage path>";
	}
	return 0;
}

DLL_PUBLIC const char** getDatabaseConfigParameters( DatabaseConfigDescriptionType type)
{
	static const char* keys_CreateDatabaseClient[] = {"path","cache","compression", 0};
	static const char* keys_CreateDatabase[] = {"path","compression", 0};
	static const char* keys_DestroyDatabase[] = {"path", 0};
	switch (type)
	{
		case CmdCreateDatabaseClient:	return keys_CreateDatabaseClient;
		case CmdCreateDatabase:		return keys_CreateDatabase;
		case CmdDestroyDatabase:	return keys_DestroyDatabase;
	}
	return 0;
}



