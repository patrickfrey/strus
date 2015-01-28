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
#include "databaseConfig.hpp"
#include "database.hpp"
#include "dll_tags.hpp"
#include <leveldb/db.h>

using namespace strus;

DLL_PUBLIC DatabaseInterface* strus::createDatabaseClient( const char* configsource)
{
	DatabaseConfig config( configsource);
	return new Database( config.path().c_str(), config.cachesize_kb(), config.compression());
}


DLL_PUBLIC void strus::createDatabase( const char* configsource)
{
	DatabaseConfig config( configsource);

	leveldb::DB* db = 0;
	leveldb::Options options;
	// Compression reduces size of index by 25% and has about 10% better performance
	// m_dboptions.compression = leveldb::kNoCompression;
	options.create_if_missing = true;
	options.error_if_exists = true;

	if (!config.compression())
	{
		options.compression = leveldb::kNoCompression;
	}
	leveldb::Status status = leveldb::DB::Open( options, config.path(), &db);
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
	DatabaseConfig config( configsource);
	leveldb::Options options;
	leveldb::Status status = leveldb::DestroyDB( config.path(), options);
	if (!status.ok())
	{
		std::string err = status.ToString();
		throw std::runtime_error( std::string( "failed to remove storage: ") + err);
	}
}


DLL_PUBLIC const char* strus::getDatabaseConfigDescription( DatabaseConfigDescriptionType type)
{
	switch (type)
	{
		case CmdCreateDatabaseClient:
			return "path=<LevelDB storage path>\ncache=<size of LRU cache for LevelDB>";

		case CmdCreateDatabase:
			return "path=<LevelDB storage path>;compression=<yes/no>";

		case CmdDestroyDatabase:
			return "path=<LevelDB storage path>";
	}
	return 0;
}




