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
#include "storage.hpp"
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include "metaDataReader.hpp"
#include "metaDataBlock.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <leveldb/db.h>

static unsigned int g_nofErrors = 0;

static void logError( const std::string& msg)
{
	std::cerr << "error: " << msg << std::endl;
	++g_nofErrors;
}
template <typename ARG>
static void logError( const std::string& msg, const ARG& arg)
{
	std::cerr << "error: " << msg << " (" << arg << ")" << std::endl;
	++g_nofErrors;
}


static std::string keystring( const leveldb::Slice& key)
{
	std::string rt;
	char const* ki = key.data();
	char const* ke = key.data()+key.size();
	for (; ki != ke; ++ki)
	{
		if (*ki > 32 && *ki < 128)
		{
			rt.push_back( *ki);
		}
		else
		{
			rt.push_back( '_');
		}
	}
	return rt;
}

static void checkKeyValue( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();
	try
	{
		switch (key.data()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				if (!strus::checkStringUtf8( ki, ke-ki))
				{
					logError( "key of term type is not a valid UTF8 string");
					break;
				}
				(void)strus::unpackIndex( vi, ve);/*[typeno]*/
				if (vi != ve)
				{
					logError( "unexpected extra bytes at end of term type number");
					break;
				}
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				if (!strus::checkStringUtf8( ki, ke-ki))
				{
					logError( "key of term value is not a valid UTF8 string");
					break;
				}
				(void)strus::unpackIndex( vi, ve);/*[valueno]*/
				if (vi != ve)
				{
					logError( "unexpected extra bytes at end of term value number");
					break;
				}
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				if (!strus::checkStringUtf8( ki, ke-ki))
				{
					logError( "key of doc id is not a valid UTF8 string");
					break;
				}
				(void)strus::unpackIndex( vi, ve);/*[docno]*/
				if (vi != ve)
				{
					logError( "unexpected extra bytes at end of document number");
					break;
				}
				break;
			}
			case strus::DatabaseKey::InvertedIndexPrefix:
			{
				(void)strus::unpackIndex( ki, ke);/*[typeno]*/
				(void)strus::unpackIndex( ki, ke);/*[valueno]*/
				(void)strus::unpackIndex( ki, ke);/*[docno]*/
				if (ki != ke)
				{
					logError( "unexpected extra bytes at end of term index key");
					break;
				}
				strus::Index ff = strus::unpackIndex( vi, ve);
				strus::Index prevpos = 0;
				strus::Index poscnt = 0;
				while (vi != ve)
				{
					strus::Index pos = strus::unpackIndex( vi, ve);
					if (prevpos >= pos)
					{
						logError( "positions not ascending in location index");
						break;
					}
					prevpos = pos;
					++poscnt;
				}
				if (ff != poscnt)
				{
					logError( "ff does not match to count of positions");
					break;
				}
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				(void)strus::unpackIndex( ki, ke);/*[docno]*/
				(void)strus::unpackIndex( ki, ke);/*[typeno]*/
				(void)strus::unpackIndex( ki, ke);/*[pos]*/
				if (ki != ke)
				{
					logError( "unexpected extra bytes at end of forward index key");
					break;
				}
				if (!strus::checkStringUtf8( vi, ve-vi))
				{
					logError( "value in addressed by forward index is not a valied UTF-8 string");
					break;
				}
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				if (!strus::checkStringUtf8( ki, ke-ki))
				{
					logError( "illegal UTF8 string as key of global variable");
					break;
				}
				(void)strus::unpackIndex( vi, ve);/*[valueno]*/
				if (vi != ve)
				{
					logError( "unexpected extra bytes at end of variable value");
					break;
				}
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				if (ki == ke)
				{
					logError( "unexpected end of metadata key");
					break;
				}
				char varname = *ki++;
				if (varname < 32 || varname > 127)
				{
					logError( "variable name in metadata key out of range");
					break;
				}
				(void)strus::unpackIndex( ki, ke);/*[blockno]*/
				if (ki != ke)
				{
					logError( "unexpected extra bytes at end of metadata key");
					break;
				}
				if ((ve - vi) != (strus::MetaDataBlock::MetaDataBlockSize*sizeof(float)))
				{
					logError( "corrupt meta data block (unexpected size of meta data block)", (unsigned int)(ve - vi));
					break;
				}
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				(void)strus::unpackIndex( ki, ke);/*[docno]*/
				if (ki == ke)
				{
					logError( "unexpected end of document attribute key");
					break;
				}
				char varname = *ki++;
				if (varname < 32 || varname > 127)
				{
					logError( "variable name in document attribute key out of range");
					break;
				}
				if (!strus::checkStringUtf8( vi, ve-vi))
				{
					logError( "value in document attribute value is not a valid UTF-8 string");
					break;
				}
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				(void)strus::unpackIndex( ki, ke);/*[typeno]*/
				(void)strus::unpackIndex( ki, ke);/*[valueno]*/
				if (ki != ke)
				{
					logError( "unexpected extra bytes at end of term document frequency key");
					break;
				}
				(void)strus::unpackIndex( vi, ve);/*[df]*/
				if (vi != ve)
				{
					logError( "unexpected extra bytes at end of df value");
					break;
				}
				break;
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		logError( err.what(), std::string("in key [") + keystring( key) + "]");
	}
}

static void checkDB( leveldb::DB* db)
{
	unsigned int cnt = 0;
	char prevkeytype = 0;
	leveldb::Iterator* itr = db->NewIterator( leveldb::ReadOptions());
	for (itr->SeekToFirst(); itr->Valid(); itr->Next())
	{
		leveldb::Slice key = itr->key();
		if (!key.data() || key.size() == 0)
		{
			logError( "found empty key in storage");
			continue;
		}
		switch (key.data()[0])
		{
			logError( "illegal data base prefix", key.data()[0]);
			continue;
		}
		if (prevkeytype != key.data()[0])
		{
			if (prevkeytype)
			{
				std::cerr << "... checked " << cnt << " entries" << std::endl;
				cnt = 0;
			}
			std::cerr << "checking entries of type '"
					<< strus::DatabaseKey::keyPrefixName(
						  (strus::DatabaseKey::KeyPrefix)key.data()[0])
					<< "':"
					<< std::endl;
			prevkeytype = key.data()[0];
		}
		checkKeyValue( key, itr->value());
		++cnt;
	};
	std::cerr << "... checked " << cnt << " entries" << std::endl;
	cnt = 0;

	if (g_nofErrors)
	{
		std::cerr << "FAILED." << g_nofErrors << " found in storage index" << std::endl;
	}
	else
	{
		std::cerr << "OK. No errors found in storage index" << std::endl;
	}
}

int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		std::cerr << "usage: strusCheck <config>" << std::endl;
		std::cerr << "<config>  : configuration string of the storage";
		std::string indent;
		char const* cc = strus::getStorageConfigDescription();
		char const* ee;
		do
		{
			ee = std::strchr( cc,'\n');
			std::string line = ee?std::string( cc, ee-cc):std::string( cc);
			std::cerr << indent << line << std::endl;
			cc = ee + 1;
			if (indent.empty())
			{
				indent = std::string( 12, ' ');
			}
		}
		while (ee);
		return 0;
	}
	try
	{
		if (argc < 2) throw std::runtime_error( "too few arguments (expected storage configuration string)");
		if (argc > 2) throw std::runtime_error( "too many arguments for strusCreate");

		leveldb::DB* db;

		const char* config = std::strstr( argv[1], "path=");
		if (!config)
		{
			throw std::runtime_error( std::string( "no path=... definition in configuration string: '") + argv[1] + "'");
		}
		const char* path = config+5;
		const char* pathend = std::strchr( path, ';');
		if (!pathend) pathend = std::strchr( path, '\0');
		
		leveldb::Options dboptions;
		dboptions.create_if_missing = false;
		leveldb::Status status = leveldb::DB::Open( dboptions, std::string( path, pathend-path), &db);
		if (status.ok())
		{
			checkDB( db);
		}
		else
		{
			std::string err = status.ToString();
			if (!!db)
			{
				delete db;
				db = 0;
			}
			throw std::runtime_error( std::string( "failed to open storage: ") + err);
		}
		
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "EXCEPTION " << e.what() << std::endl;
	}
	return -1;
}


