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
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include "storage.hpp"
#include "dll_tags.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace strus;

namespace {
class CaseInsensitiveKey
	:public std::string
{
public:
	CaseInsensitiveKey(){}
	CaseInsensitiveKey( const std::string& o)
		:std::string( boost::algorithm::to_lower_copy( o)){}
	CaseInsensitiveKey( const CaseInsensitiveKey& o)
		:std::string( o){}
};

class ConfigMap
	:public std::map<CaseInsensitiveKey,std::string>
{
public:
	ConfigMap( const char* config);
};

ConfigMap::ConfigMap( const char* config)
{
	char const* cc = config;
	while (*cc)
	{
		std::string key;
		while ((*cc|32) >= 'a' && (*cc|32) <= 'z')
		{
			key.push_back( *cc++);
		}
		if (*cc != '=')
		{
			throw std::runtime_error( "'=' expected after item identifier in config string");
		}
		++cc;
		const char* ee = std::strchr( cc, ';');
		if (!ee) ee = std::strchr( cc, '\0');
		(*this)[ key] = std::string( cc,ee-cc);
		cc = (*ee)?(ee+1):ee;
	}
}
}//namespace


static unsigned int nofK( const std::string& numstr)
{
	Index rt = 0;
	char const* cc = numstr.c_str();
	for (;*cc; ++cc)
	{
		if (*cc >= '0' && *cc <= '9')
		{
			rt = (rt * 10) + (*cc - '0');
		}
		else if (*cc == 'K' || *cc == 'k')
		{
			rt = rt * 1024;
			++cc;
			break;
		}
		else if (*cc == 'M' || *cc == 'm')
		{
			rt = rt * 1024 * 1024;
			++cc;
			break;
		}
		else if (*cc == 'G' || *cc == 'g')
		{
			rt = rt * 1024 * 1024 * 1024;
			++cc;
			break;
		}
	}
	if (!cc)
	{
		throw std::runtime_error( std::string( "not a number (with optional 'K' or 'M' or 'G' suffix) for configuration option 'cache': '") + numstr + "'");
	}
	return (unsigned int)((rt + 1023)/1024);
}


DLL_PUBLIC StorageInterface* strus::createStorageClient( const char* config)
{
	std::string path;
	unsigned int cachesize_k = 0;

	ConfigMap configMap( config);
	ConfigMap::const_iterator ci = configMap.begin(), ce = configMap.end();
	for (; ci != ce; ++ci)
	{
		if (ci->first == "path")
		{
			path = ci->second;
		}
		else if (ci->first == "cache")
		{
			cachesize_k = nofK( ci->second);
		}
		else
		{
			throw std::runtime_error( std::string( "unknown configuration option '") + ci->first +"'");
		}
	}
	if (path.empty())
	{
		throw std::runtime_error( "no path defined in config for levelDB storage");
	}
	return new Storage( path.c_str(), cachesize_k);
}


DLL_PUBLIC void strus::createStorageDatabase( const char* config)
{
	std::string path;
	ConfigMap configMap( config);
	ConfigMap::const_iterator ci = configMap.begin(), ce = configMap.end();
	for (; ci != ce; ++ci)
	{
		if (ci->first == "path")
		{
			path = ci->second;
		}
		else if (ci->first == "cache")
		{
		}
		else
		{
			throw std::runtime_error( std::string( "unknown configuration option '") + ci->first +"'");
		}
	}
	if (path.empty())
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
		batch.Put( Storage::keyString( DatabaseKey::VariablePrefix, "TermNo"), "\1");
		batch.Put( Storage::keyString( DatabaseKey::VariablePrefix, "TypeNo"), "\1");
		batch.Put( Storage::keyString( DatabaseKey::VariablePrefix, "DocNo"), "\1");
		batch.Put( Storage::keyString( DatabaseKey::VariablePrefix, "NofDocs"), "\1");

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


