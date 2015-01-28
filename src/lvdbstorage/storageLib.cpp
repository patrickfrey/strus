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
#include "strus/storageLib.hpp"
#include "strus/storageInterface.hpp"
#include "storageAlterMetaDataTable.hpp"
#include "storageConfig.hpp"
#include "databaseKey.hpp"
#include "keyMap.hpp"
#include "indexPacker.hpp"
#include "storage.hpp"
#include "dll_tags.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <boost/algorithm/string.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace strus;

static std::string loadFile( const std::string& filename)
{
	std::string rt;
	int errcode = 0;

	FILE* fh = ::fopen( filename.c_str(), "rb");
	if (!fh)
	{
		errcode = errno;
		goto FILEERROR;
	}
	unsigned int nn;
	enum {bufsize=(1<<12)};
	char buf[ bufsize];

	while (!!(nn=::fread( buf, 1, bufsize, fh)))
	{
		try
		{
			rt.append( buf, nn);
		}
		catch (const std::bad_alloc&)
		{
			errcode = 12/*ENOMEM*/;
			goto FILEERROR;
		}
	}
	if (!feof( fh))
	{
		errcode = ::ferror( fh);
		goto FILEERROR;
	}
	else
	{
		::fclose( fh);
	}
	return rt;
FILEERROR:
	{
		std::ostringstream errstr;
		errstr << errcode;
		if (fh) ::fclose( fh);
		throw std::runtime_error( std::string( "could not read file '") + filename + "': (errno " + errstr.str() + ")");
	}
}


DLL_PUBLIC StorageInterface* strus::createStorageClient( const char* configsource)
{
	StorageConfig config( configsource);
	if (config.metadata().size())
	{
		throw std::runtime_error( "meta data definitions only allowed when creating a storage");
	}
	if (config.cachedterms().size())
	{
		std::string cachedtermsrc = loadFile( config.cachedterms());
		return new Storage( config.path().c_str(), config.cachesize_kb(), config.compression(), cachedtermsrc.c_str());
	}
	else
	{
		return new Storage( config.path().c_str(), config.cachesize_kb(), config.compression());
	}
}


DLL_PUBLIC void strus::createStorageDatabase( const char* configsource)
{
	StorageConfig config( configsource);

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
		leveldb::WriteBatch batch;
		KeyValueStorage varstor( db, DatabaseKey::VariablePrefix, false);
	
		varstor.store( "TermNo", std::string("\1"), batch);
		varstor.store( "TypeNo", std::string("\1"), batch);
		varstor.store( "DocNo", std::string("\1"), batch);
		if (config.acl())
		{
			varstor.store( "UserNo", std::string("\1"), batch);
		}
		varstor.store( "AttribNo", std::string("\1"), batch);
		varstor.store( "NofDocs", std::string("\0",1), batch);

		MetaDataDescription md( config.metadata());
		md.store( batch);

		status = db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			std::string err = status.ToString();
			(void)leveldb::DestroyDB( config.path(), leveldb::Options());
			delete db;
			throw std::runtime_error( std::string( "failed to write to created storage: ") + err);
		}

	}
	else
	{
		std::string err = status.ToString();
		if (db) delete db;
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
	if (db) delete db;
}


DLL_PUBLIC void strus::destroyStorageDatabase( const char* configsource)
{
	StorageConfig config( configsource);
	leveldb::Options options;
	leveldb::Status status = leveldb::DestroyDB( config.path(), options);
	if (!status.ok())
	{
		std::string err = status.ToString();
		throw std::runtime_error( std::string( "failed to remove storage: ") + err);
	}
}


DLL_PUBLIC const char* strus::getStorageConfigDescription( StorageConfigDescriptionType type)
{
	switch (type)
	{
		case CmdCreateStorageClient:
			return "semicolon separated list of assignments:\npath=<LevelDB storage path>\ncache=<size of LRU cache for LevelDB>\ncachedterms=<file with list of terms to cache>;compression=<yes/no>";

		case CmdCreateStorageDatabase:
			return "semicolon separated list of assignments:\npath=<LevelDB storage path>\nacl=<yes/no, yes if users with different access rights exist>\nmetadata=<comma separated list of meta data def>;compression=<yes/no>";

		case CmdDestroyStorageDatabase:
			return "assignment:\npath=<LevelDB storage path>";
	}
	return 0;
}


DLL_PUBLIC StorageAlterMetaDataTableInterface* strus::createAlterMetaDataTable( const char* configsource)
{
	StorageConfig config( configsource);
	return new StorageAlterMetaDataTable( config.path().c_str());
}


