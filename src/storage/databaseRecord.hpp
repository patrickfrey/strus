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
#ifndef _STRUS_DATABASE_RECORD_HPP_INCLUDED
#define _STRUS_DATABASE_RECORD_HPP_INCLUDED
#include "strus/index.hpp"
#include "databaseKey.hpp"
#include <utility>
#include <string>
#include <cstring>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;

class DatabaseRecord_StringIndex_Base
{
public:
	static Index get( char prefix, const DatabaseInterface* database, const std::string& key);
	static bool load( char prefix, const DatabaseInterface* database, const std::string& key, Index& value);
	static void store( char prefix, DatabaseTransactionInterface* transaction, const std::string& key, const Index& value);
	static void remove( char prefix, DatabaseTransactionInterface* transaction, const std::string& key);
	static void storeImm( char prefix, DatabaseInterface* database, const std::string& key, const Index& value);
};

template <char KeyPrefix>
class DatabaseRecord_StringIndex
	:public DatabaseRecord_StringIndex_Base
{
public:
	static Index get( const DatabaseInterface* database, const std::string& key)
	{
		return DatabaseRecord_StringIndex_Base::get( KeyPrefix, database, key);
	}
	static bool load( const DatabaseInterface* database, const std::string& key, Index& value)
	{
		return DatabaseRecord_StringIndex_Base::load( KeyPrefix, database, key, value);
	}
	static void store( DatabaseTransactionInterface* transaction, const std::string& key, const Index& value)
	{
		DatabaseRecord_StringIndex_Base::store( KeyPrefix, transaction, key, value);
	}
	static void remove( DatabaseTransactionInterface* transaction, const std::string& key)
	{
		DatabaseRecord_StringIndex_Base::remove( KeyPrefix, transaction, key);
	}
	static void storeImm( DatabaseInterface* database, const std::string& key, const Index& value)
	{
		DatabaseRecord_StringIndex_Base::storeImm( KeyPrefix, database, key, value);
	}
};

class DatabaseRecord_TermType
	:public DatabaseRecord_StringIndex<DatabaseKey::TermTypePrefix>{};

class DatabaseRecord_TermValue
	:public DatabaseRecord_StringIndex<DatabaseKey::TermValuePrefix>{};

class DatabaseRecord_DocId
	:public DatabaseRecord_StringIndex<DatabaseKey::DocIdPrefix>{};

class DatabaseRecord_Variable
	:public DatabaseRecord_StringIndex<DatabaseKey::VariablePrefix>{};

class DatabaseRecord_AttributeKey
	:public DatabaseRecord_StringIndex<DatabaseKey::AttributeKeyPrefix>{};

class DatabaseRecord_UserName
	:public DatabaseRecord_StringIndex<DatabaseKey::UserNamePrefix>{};

class DatabaseRecord_ForwardIndex
{
public:
private:
	enum {KeyPrefix=DatabaseKey::ForwardIndexPrefix};
};

class DatabaseRecord_PosinfoBlock
{
public:
private:
	enum {KeyPrefix=DatabaseKey::PosinfoBlockPrefix};
};

class DatabaseRecord_InverseTerm
{
public:
private:
	enum {KeyPrefix=DatabaseKey::InverseTermPrefix};
};

class DatabaseRecord_UserAclBlock
{
public:
private:
	enum {KeyPrefix=DatabaseKey::UserAclBlockPrefix};
};

class DatabaseRecord_AclBlock
{
public:
private:
	enum {KeyPrefix=DatabaseKey::AclBlockPrefix};
};

class DatabaseRecord_DocListBlock
{
public:
private:
	enum {KeyPrefix=DatabaseKey::DocListBlockPrefix};
};

class DatabaseRecord_DocMetaData
{
public:
	static bool load( const DatabaseInterface* database, const Index& blockno, std::string& blk);
	static void store( DatabaseTransactionInterface* transaction, const Index& blockno, const char* blk, std::size_t blksize);
	static void remove( DatabaseTransactionInterface* transaction, const Index& blockno);

private:
	enum {KeyPrefix=DatabaseKey::DocMetaDataPrefix};
};

class DatabaseRecord_DocAttribute
{
public:
	static bool load( const DatabaseInterface* database, const Index& docno, const Index& attrno, std::string& value);
	static void store( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno, const char* value);
	static void remove( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno);
	static void removeAll( DatabaseTransactionInterface* transaction, const Index& docno);

private:
	enum {KeyPrefix=DatabaseKey::DocAttributePrefix};
};

class DatabaseRecord_DocFrequency
{
public:
	static Index get( const DatabaseInterface* database, const Index& typeno, const Index& termno);
	static bool load( const DatabaseInterface* database, const Index& typeno, const Index& termno, Index& df);
	static void store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& df);
	static void remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno);
	static void storeImm( DatabaseInterface* database, const Index& typeno, const Index& termno, const Index& df);
private:
	enum {KeyPrefix=DatabaseKey::DocFrequencyPrefix};
};

class DatabaseRecord_MetaDataDescr
{
public:
	static bool load( const DatabaseInterface* database, std::string& descr);
	static void store( DatabaseTransactionInterface* transaction, const std::string& descr);
	static void storeImm( DatabaseInterface* database, const std::string& descr);
private:
	enum {KeyPrefix=DatabaseKey::MetaDataDescrPrefix};
};

}//namespace
#endif


