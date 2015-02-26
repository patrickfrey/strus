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
#include <boost/algorithm/string.hpp>
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

static void dumpKeyValue(
		std::ostream& out,
		const strus::MetaDataDescription* metadescr,
		const strus::DatabaseCursorInterface::Slice& key,
		const strus::DatabaseCursorInterface::Slice& value)
{
	try
	{
		switch (key.ptr()[0])
		{
			case strus::DatabaseKey::TermTypePrefix:
			{
				strus::TermTypeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::TermValuePrefix:
			{
				strus::TermValueData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocIdPrefix:
			{
				strus::DocIdData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::ForwardIndexPrefix:
			{
				strus::ForwardIndexData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::VariablePrefix:
			{
				strus::VariableData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocMetaDataPrefix:
			{
				strus::DocMetaDataData data( metadescr, key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocAttributePrefix:
			{
				strus::DocAttributeData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserNamePrefix:
			{
				strus::UserNameData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocFrequencyPrefix:
			{
				strus::DocFrequencyData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::PosinfoBlockPrefix:
			{
				strus::PosinfoBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::UserAclBlockPrefix:
			{
				strus::UserAclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AclBlockPrefix:
			{
				strus::AclBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::DocListBlockPrefix:
			{
				strus::DocListBlockData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::InverseTermPrefix:
			{
				strus::InverseTermData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::MetaDataDescrPrefix:
			{
				strus::MetaDataDescrData data( key, value);
				data.print( out);
				break;
			}
			case strus::DatabaseKey::AttributeKeyPrefix:
			{
				strus::AttributeKeyData data( key, value);
				data.print( out);
				break;
			}
			default:
			{
				logError( "illegal data base prefix", key.ptr()[0]);
				break;
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		logError( err.what(), std::string("in key [") + keystring( key) + "]");
	}
}

static void dumpDB( std::ostream& out, strus::DatabaseClientInterface* database)
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
				std::cerr << "... dumped " << cnt << " entries" << std::endl;
				cnt = 0;
			}
			std::cerr << "dumping entries of type '"
				<< strus::DatabaseKey::keyPrefixName(
					  (strus::DatabaseKey::KeyPrefix)key.ptr()[0])
				<< "':"
				<< std::endl;
			prevkeytype = key.ptr()[0];
		}
		dumpKeyValue( out, &metadescr, key, cursor->value());
		++cnt;
	};
	std::cerr << "... dumped " << cnt << " entries" << std::endl;
	cnt = 0;
}

static void dumpDB( std::ostream& out, strus::DatabaseClientInterface* database, char keyprefix)
{
	if (!keyprefix)
	{
		dumpDB( out, database);
		return;
	}
	strus::MetaDataDescription metadescr( database);
	boost::scoped_ptr<strus::DatabaseCursorInterface>
		cursor( database->createCursor( strus::DatabaseOptions()));

	strus::DatabaseCursorInterface::Slice key = cursor->seekFirst( &keyprefix, 1);
	unsigned int cnt = 0;
	for (; key.defined(); key = cursor->seekNext())
	{
		dumpKeyValue( out, &metadescr, key, cursor->value());
		++cnt;
	};
	std::cerr << "... dumped " << cnt << " entries" << std::endl;
	cnt = 0;
}


static char getDatabaseKeyPrefix( const char* name)
{
	if (boost::algorithm::iequals( name, "termtype")) return (char)strus::DatabaseKey::TermTypePrefix;
	if (boost::algorithm::iequals( name, "termvalue")) return (char)strus::DatabaseKey::TermValuePrefix;
	if (boost::algorithm::iequals( name, "docid")) return (char)strus::DatabaseKey::DocIdPrefix;
	if (boost::algorithm::iequals( name, "variable")) return (char)strus::DatabaseKey::VariablePrefix;
	if (boost::algorithm::iequals( name, "attrname")) return (char)strus::DatabaseKey::AttributeKeyPrefix;
	if (boost::algorithm::iequals( name, "username")) return (char)strus::DatabaseKey::UserNamePrefix;
	if (boost::algorithm::iequals( name, "forward")) return (char)strus::DatabaseKey::ForwardIndexPrefix;
	if (boost::algorithm::iequals( name, "posinfo")) return (char)strus::DatabaseKey::PosinfoBlockPrefix;
	if (boost::algorithm::iequals( name, "invterm")) return (char)strus::DatabaseKey::InverseTermPrefix;
	if (boost::algorithm::iequals( name, "useracl")) return (char)strus::DatabaseKey::UserAclBlockPrefix;
	if (boost::algorithm::iequals( name, "docacl")) return (char)strus::DatabaseKey::AclBlockPrefix;
	if (boost::algorithm::iequals( name, "doclist")) return (char)strus::DatabaseKey::DocListBlockPrefix;
	if (boost::algorithm::iequals( name, "metadata")) return (char)strus::DatabaseKey::DocMetaDataPrefix;
	if (boost::algorithm::iequals( name, "docattr")) return (char)strus::DatabaseKey::DocAttributePrefix;
	if (boost::algorithm::iequals( name, "df")) return (char)strus::DatabaseKey::DocFrequencyPrefix;
	if (boost::algorithm::iequals( name, "metatable")) return (char)strus::DatabaseKey::MetaDataDescrPrefix;
	throw std::runtime_error( std::string("unknown database key type '") + name + "'");
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
		std::cerr << "usage: strusDumpStorage [options] <config> [ <what> ]" << std::endl;
		std::cerr << "<config>  : configuration string of the storage;" << std::endl;
		std::cerr << "            semicolon ';' separated list of assignments:" << std::endl;

		strus::printIndentMultilineString(
					std::cerr,
					12, db->getConfigDescription(
						strus::DatabaseInterface::CmdCreateClient));
		std::cerr << "<what>    : optional name of entries to dump:" << std::endl;
		std::cerr << "            termtype  :term type definitions" << std::endl;
		std::cerr << "            termvalue :term value definitions" << std::endl;
		std::cerr << "            docid     :document identifier definitions" << std::endl;
		std::cerr << "            variable  :variable definitions" << std::endl;
		std::cerr << "            attrname  :attribute name definitions" << std::endl;
		std::cerr << "            username  :user name definitions" << std::endl;
		std::cerr << "            forward   :forward index blocks" << std::endl;
		std::cerr << "            docno     :document statistic blocks" << std::endl;
		std::cerr << "            posinfo   :posinfo index blocks" << std::endl;
		std::cerr << "            invterm   :inverse term index blocks" << std::endl;
		std::cerr << "            useracl   :user ACL index blocks" << std::endl;
		std::cerr << "            docacl    :document ACL index blocks" << std::endl;
		std::cerr << "            doclist   :term document index blocks" << std::endl;
		std::cerr << "            metadata  :meta data blocks" << std::endl;
		std::cerr << "            docattr   :document attributes" << std::endl;
		std::cerr << "            df        :term document frequency definitions" << std::endl;
		std::cerr << "            metatable :meta data element descriptions" << std::endl;
		std::cerr << "options:" << std::endl;
		std::cerr << "-h,--help     : Print this usage info and exit" << std::endl;
		std::cerr << "-v,--version  : Print the version info and exit" << std::endl;
		return 0;
	}
	try
	{
		if (argc < 2) throw std::runtime_error( "too few arguments (expected storage configuration string)");
		if (argc > 3) throw std::runtime_error( "too many arguments for strusDumpStorage");

		char keyprefix = 0;
		if (argc == 3)
		{
			keyprefix = getDatabaseKeyPrefix( argv[2]);
		}
		boost::scoped_ptr<strus::DatabaseClientInterface>
			database( db->createClient( argv[1]));

		dumpDB( std::cout, database.get(), keyprefix);
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


