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
#include "indexPacker.hpp"
#include "storage.hpp"
#include "dll_tags.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace strus;

static const char* configGet( const char* config, const char* name)
{
	const char* cc = config;
	std::size_t namelen = std::strlen(name);
	while (0!=std::memcmp( cc, name, namelen) || cc[namelen] != '=')
	{
		cc = std::strchr( cc, ';');
		if (!cc) break;
		cc = cc + 1;
	}
	return cc?(cc+namelen+1):0;
}


DLL_PUBLIC StorageInterface* strus::createStorageClient( const char* config)
{
	const char* path = configGet( config, "path");
	if (!path)
	{
		throw std::runtime_error( "no path defined in config for levelDB storage");
	}
	return new Storage( path);
}

DLL_PUBLIC void strus::createStorageDatabase( const char* config)
{
	const char* path = configGet( config, "path");
	if (!path)
	{
		throw std::runtime_error( "no path defined in config for levelDB storage");
	}
	leveldb::DB* db = 0;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = true;
	leveldb::Status status = leveldb::DB::Open( options, path, &db);
	if (status.ok())
	{
		leveldb::WriteBatch batch;
		batch.Put( Storage::keyString( Storage::VariablePrefix, "TermNo"), "\1");
		batch.Put( Storage::keyString( Storage::VariablePrefix, "TypeNo"), "\1");
		batch.Put( Storage::keyString( Storage::VariablePrefix, "DocNo"), "\1");
		batch.Put( Storage::keyString( Storage::VariablePrefix, "NofDocs"), "\0");
		status = db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			std::string err = status.ToString();
			(void)leveldb::DestroyDB( path, leveldb::Options());
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

DLL_PUBLIC const char* strus::getStorageConfigDescription()
{
	return "assignment of the LevelDB storage path as string\n(example \"path=data/testdb\")";
}


