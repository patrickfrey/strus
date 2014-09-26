#include "strus/storageInterface.hpp"
#include "indexPacker.hpp"

/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#include "strus/storageInterface.hpp"
#include "dll_tags.hpp"
#include <string>
#include <cstring>
#include <kcplantdb.h>

namespace strus
{

class StorageKyotoCabinet
	:public StorageInterface
{
public:
	StorageKyotoCabinet();
	virtual ~StorageKyotoCabinet(){}

	Index termId( const std::string& type, const std::string& id);

	virtual IteratorInterfaceR
		termOccurrenceIterator(
			const std::string& termtype,
			const std::string& termid);

private:
	std::string m_name;
	kyotocabinet::ForestDB m_db;
	bool m_allowWrite;
};

static const char* pathItemSeparator()
{
#if defined _WIN32
	return  "\\";
#else
	return "/";
#endif
}

StorageKyotoCabinet::StorageKyotoCabinet( const char* directory_, const char* name_, bool allowWrite_)
	:m_name(name_)
	,m_allowWrite(allowWrite_)
{
	std::string path;
	if (directory_)
	{
		path.append( directory_);
		path.append( pathItemSeparator());
	}
	path.append( m_name);
	path.append( ".kch");

	if (doWrite)
	{
		if (!m_db.open( path, kyotocabinet::ForestDB::OWRITER))
		{
			throw std::runtime_error( std::string( "kcstorage open (for read/write) error: ") + m_db.error().name());
		}
	}
	else
	{
		if (!m_db.open( path, kyotocabinet::ForestDB::OREADER))
		{
			throw std::runtime_error( std::string( "kcstorage open (for read) error: ") + m_db.error().name());
		}
	}

	
}


         // store records
         if (!db.set("foo", "hop") ||
             !db.set("bar", "step") ||
             !db.set("baz", "jump")) {
           cerr << "set error: " << db.error().name() << endl;
         }
       
         // retrieve a record
         string value;
         if (db.get("foo", &value)) {
           cout << value << endl;
         } else {
           cerr << "get error: " << db.error().name() << endl;
         }
       
         // traverse records
         DB::Cursor* cur = db.cursor();
         cur->jump();
         string ckey, cvalue;
         while (cur->get(&ckey, &cvalue, true)) {
           cout << ckey << ":" << cvalue << endl;
         }
         delete cur;
       
       
         return 0;
}

virtual StorageKyotoCabinet::~StorageKyotoCabinet()
{
	// close the database
	(void)m_db.close();
}

virtual IteratorInterfaceR
	StorageKyotoCabinet::termOccurrenceIterator(
		const std::string& termtype,
		const std::string& termid)
{
}


static const char* configGet( const char* config, const char* name)
{
	const char* cc = config;
	std::size_t namelen = std::strlen(name);
	while (0!=std::strcmp( cc, name) || cc[namelen] != '=')
	{
		cc = std::strchr( cc, ';');
		if (!cc) break;
		cc = cc + 1;
	}
	return 0;
}

DLL_PUBLIC StorageInterface* strus::createStorage( const char* config)
{
	const char* name = configGet( config, "name");
	if (!name)
	{
		throw std::runtime_error( "no storage name defined for kcstorage");
	}
	return new StorageKyotoCabinet( name);
}

DLL_PUBLIC void createStorageDatabase( const char* config)
{
	
}

}//namespace
#endif



