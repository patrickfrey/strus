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
#include "strus/lib/database_leveldb.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/databaseOptions.hpp"
#include "strus/versionStorage.hpp"
#include "extractKeyValueData.hpp"
#include "databaseKey.hpp"
#include "strus/private/cmdLineOpt.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <boost/scoped_ptr.hpp>

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


static std::string keystring( const strus::DatabaseCursorInterface::Slice& key)
{
	std::string rt;
	char const* ki = key.ptr();
	char const* ke = key.ptr()+key.size();
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

static void checkKeyValue( const strus::MetaDataDescription* metadescr,
				const strus::DatabaseCursorInterface::Slice& key,
				const strus::DatabaseCursorInterface::Slice& value)
{
	try
	{
		switch (key.ptr()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				strus::TermTypeData( key, value);
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				strus::TermValueData( key, value);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData( key, value);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData( key, value);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData( key, value);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::DocMetaDataData( metadescr, key, value);
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				strus::DocAttributeData( key, value);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData( key, value);
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData( key, value);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData( key, value);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData( key, value);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData( key, value);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData( key, value);
				break;
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		logError( err.what(), std::string("in key [") + keystring( key) + "]");
	}
}

static void checkDB( strus::DatabaseClientInterface* database)
{
	strus::MetaDataDescription metadescr( database);
	boost::scoped_ptr<strus::DatabaseCursorInterface>
		cursor( database->createCursor( strus::DatabaseOptions()));

	strus::DatabaseCursorInterface::Slice key = cursor->seekFirst( 0, 0);
	char prevkeytype = 0;
	unsigned int cnt = 0;

	for (; key.defined(); key = cursor->seekNext())
	{
		if (key.size() == 0)
		{
			logError( "found empty key in storage");
			continue;
		}
		if (prevkeytype != key.ptr()[0])
		{
			if (prevkeytype)
			{
				std::cerr << "... checked " << cnt << " entries" << std::endl;
				cnt = 0;
			}
			std::cerr << "checking entries of type '"
					<< strus::DatabaseKey::keyPrefixName(
						  (strus::DatabaseKey::KeyPrefix)key.ptr()[0])
					<< "':"
					<< std::endl;
			prevkeytype = key.ptr()[0];
		}
		checkKeyValue( &metadescr, key, cursor->value());
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
	if (argc > 1 && (std::strcmp( argv[1], "-v") == 0 || std::strcmp( argv[1], "--version") == 0))
	{
		std::cout << "Strus storage version " << STRUS_STORAGE_VERSION_STRING << std::endl;
		return 0;
	}
	const strus::DatabaseInterface* db = strus::getDatabase_leveldb();
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		std::cerr << "usage: strusCheckStorage [options] <config>" << std::endl;
		std::cerr << "<config>  : configuration string of the storage:" << std::endl;
		std::cerr << "            semicolon ';' separated list of assignments:" << std::endl;

		strus::printIndentMultilineString(
					std::cerr,
					12, db->getConfigDescription(
						strus::DatabaseInterface::CmdCreateClient));
		std::cerr << "options:" << std::endl;
		std::cerr << "-h,--help     : Print this usage info and exit" << std::endl;
		std::cerr << "-v,--version  : Print the version info and exit" << std::endl;
		return 0;
	}
	try
	{
		if (argc < 2) throw std::runtime_error( "too few arguments (expected storage configuration string)");
		if (argc > 2) throw std::runtime_error( "too many arguments for strusCheckStorage");

		boost::scoped_ptr<strus::DatabaseClientInterface>
			database( db->createClient( argv[1]));

		checkDB( database.get());
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


