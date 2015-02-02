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
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/reference.hpp"
#include "strus/private/configParser.hpp"
#include "storageAlterMetaDataTable.hpp"
#include "databaseAdapter.hpp"
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


DLL_PUBLIC StorageInterface* strus::createStorageClient( const char* configsource, DatabaseInterface* database)
{
	std::string cachedterms;
	std::string src( configsource?configsource:"");

	(void)extractStringFromConfigString( cachedterms, src, "cachedterms");
	if (cachedterms.size())
	{
		std::string cachedtermsrc = loadFile( cachedterms);
		return new Storage( database, cachedtermsrc.c_str());
	}
	else
	{
		return new Storage( database);
	}
}


DLL_PUBLIC void strus::createStorage( const char* configsource, DatabaseInterface* database)
{
	bool useAcl = false;
	std::string metadata;

	std::string src( configsource?configsource:"");

	(void)extractStringFromConfigString( metadata, src, "metadata");
	(void)extractBooleanFromConfigString( useAcl, src, "acl");

	MetaDataDescription md( metadata);
	Reference<DatabaseTransactionInterface> transaction( database->createTransaction());

	DatabaseAdapter_Variable stor( database);
	
	stor.store( transaction.get(), "TermNo", 1);
	stor.store( transaction.get(), "TypeNo", 1);
	stor.store( transaction.get(), "DocNo", 1);
	stor.store( transaction.get(), "AttribNo", 1);
	stor.store( transaction.get(), "NofDocs", 0);
	if (useAcl)
	{
		stor.store( transaction.get(), "UserNo", 1);
	}
	md.store( transaction.get());

	transaction->commit();
}


DLL_PUBLIC const char* strus::getStorageConfigDescription( StorageConfigDescriptionType type)
{
	switch (type)
	{
		case CmdCreateStorageClient:
			return "cachedterms=<file with list of terms to cache>";

		case CmdCreateStorage:
			return "acl=<yes/no, yes if users with different access rights exist>\nmetadata=<comma separated list of meta data def>";
	}
	return 0;
}


DLL_PUBLIC StorageAlterMetaDataTableInterface*
	strus::createAlterMetaDataTable( DatabaseInterface* database)
{
	return new StorageAlterMetaDataTable( database);
}

DLL_PUBLIC const char** strus::getStorageConfigParameters( StorageConfigDescriptionType type)
{
	static const char* keys_CreateStorageClient[]	= {"cachedterms", 0};
	static const char* keys_CreateStorage[]		= {"acl", "metadata", 0};
	switch (type)
	{
		case CmdCreateStorageClient:	return keys_CreateStorageClient;
		case CmdCreateStorage:		return keys_CreateStorage;
	}
	return 0;
}

