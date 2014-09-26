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
#include "strus/storageInterface.hpp"
#include "dll_tags.hpp"
#include <string>
#include <cstring>
#include "leveldb/db.h"

namespace strus
{

class StorageLevelDb
	:public StorageInterface
{
public:
	StorageLevelDb( const char* path_);
	virtual ~StorageLevelDb();

	Index termId( const std::string& type, const std::string& id);

	virtual IteratorInterfaceR
		termOccurrenceIterator(
			const std::string& termtype,
			const std::string& termid);

public:
	enum KeyPrefix
	{
		TermTypePrefix='t',
		TermIdPrefix='i',
		TermDocPrefix='o'
	};

	Index keyLookUp( KeyPrefix prefix, const std::string& str);

private:
	std::string m_path;
	leveldb::DB* m_db;
};


StorageLevelDb::StorageLevelDb( const char* path_)
	:m_path(path_),m_db(0)
{
	leveldb::Options options;
	options.create_if_missing = false;

	leveldb::Status status = leveldb::DB::Open(options, path_, &m_db);
	if (!status.ok())
	{
		std::string err = s.ToString();
		if (!db) delete db;
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}

Index StorageLevelDb::keyLookUp( KeyPrefix prefix, const std::string& str)
{
	std::string key;
	key.push_back( (char)prefix);
	key.append( str);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (!status.ok())
	{
		throw std::runtime_error( s.ToString());
	}
	if (status.IsNotFound()) return 0;
	return unpackIndex( value.begin(), value.end());
}

virtual StorageLevelDb::~StorageLevelDb()
{
	if (m_db) delete m_db;
}

class IteratorLevelDb
	:public IteratorInterface
{
public:
	IteratorLevelDb( leveldb::DB* db_, Index termtypeno, Index termidno)
		:m_db(db_),m_itr(0),m_posno(0),m_positr(0),m_posend(0)
	{
		m_key.push_back( (char)TermDocPrefix);
		packIndex( m_key, termtypeno);
		packIndex( m_key, termidno);
		m_keysize = m_key.size();
	}

	virtual ~IteratorLevelDb()
	{
		if (m_itr) delete m_itr;
	}


	virtual Index skipDoc( const Index& docno)
	{
		if (m_itr && m_docno +1 == docno)
		{
			return getNextTermDoc();
		}
		else
		{
			return getFirstTermDoc();
		}
	}

	virtual Index skipPos( const Index& firstpos)
	{
		if (m_posno > firstpos)
		{
			m_posno = 0;
			m_positr = m_itr->value().data();
			m_posend = m_positr + m_itr->value().size();
		}
		while (m_positr != m_posend && firstpos > m_posno)
		{
			// Get the next position increment and with it the next position number:
			Index incr = unpackIndex( m_positr, m_posend);
			m_posno += incr;
		}
		if (firstpos > m_posno)
		{
			return 0;
		}
		return m_posno;
	}

private:
	Index extractMatchDocno()
	{
		if (m_keysize < m_itr->size() && 0==std::memcmp( m_key.c_str(), m_itr->key().data(), m_keysize))
		{
			// Check if we are still on the same term:
			const char* ki = m_itr->key().data();
			const char* ke = ki + m_itr->key().size();

			m_posno = 0;
			m_positr = m_itr->value().data();
			m_posend = m_positr + m_itr->value().size();

			// Return the matching document number:
			return unpackIndex( ki, ke);
		}
		else
		{
			delete m_itr;
			m_itr = 0;
			m_posno = 0;
			m_positr = 0;
			m_posend = 0;
			return 0;
		}
	}

	Index getNextTermDoc()
	{
		m_itr->Next();
		return extractMatchDocno();
	}

	Index getFirstTermDoc( const Index& docno)
	{
		if (!m_itr)
		{
			m_itr = db->NewIterator( leveldb::ReadOptions());
		}
		m_key.resize( m_keysize);
		packIndex( m_key, docno);
		m_itr->Seek( leveldb::Slice( m_key.c_str(), m_keysize));

		return extractMatchDocno();
	}

private:
	leveldb::DB* m_db;
	std::string m_key;
	std::size_t m_keysize;
	Index m_docno;
	leveldb::Iterator* m_itr;
	Index m_posno;
	const char* m_positr;
	const char* m_posend;
};

virtual IteratorInterfaceR
	StorageLevelDb::termOccurrenceIterator(
		const std::string& termtype,
		const std::string& termid)
{
	return IteratorInterfaceR( new IteratorLevelDb( m_db, termtypeno( termtype), termidno( termtype)));
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
	return new StorageLevelDb( name);
}

DLL_PUBLIC void createStorageDatabase( const char* config)
{
	const char* path = configGet( config, "path");
	if (!path)
	{
		throw std::runtime_error( "no storage name defined for storage");
	}
	leveldb::DB* db = 0;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = true;
	leveldb::Status status = leveldb::DB::Open( options, path, &db);
	if (!status.ok())
	{
		std::string err = s.ToString();
		if (!db) delete db;
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}

}//namespace
#endif



