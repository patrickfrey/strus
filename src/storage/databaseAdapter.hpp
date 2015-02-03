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
#ifndef _STRUS_DATABASE_ADAPTER_HPP_INCLUDED
#define _STRUS_DATABASE_ADAPTER_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/reference.hpp"
#include "databaseKey.hpp"
#include "blockKey.hpp"
#include <utility>
#include <string>
#include <cstring>

namespace strus {

/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class MetaDataBlock;
/// \brief Forward declaration
class MetaDataDescription;
/// \brief Forward declaration
class PosinfoBlock;
/// \brief Forward declaration
class BooleanBlock;
/// \brief Forward declaration
class InvTermBlock;
/// \brief Forward declaration
class DataBlock;
/// \brief Forward declaration
class ForwardIndexBlock;

class DatabaseAdapter_StringIndex
{
public:
	class Cursor
	{
	public:
		Cursor( char prefix_, DatabaseInterface* database_)
			:m_cursor( database_->createCursor(false)),m_prefix(prefix_){}

		bool loadFirst( std::string& key, Index& value);
		bool loadNext( std::string& key, Index& value);

	private:
		bool getData( const DatabaseCursorInterface::Slice& dbkey, std::string& key, Index& value);

	private:
		Reference<DatabaseCursorInterface> m_cursor;
		char m_prefix;
	};

	DatabaseAdapter_StringIndex( char prefix_, DatabaseInterface* database_)
		:m_prefix(prefix_),m_database(database_){}

	Index get( const std::string& key);
	bool load( const std::string& key, Index& value);
	void store( DatabaseTransactionInterface* transaction, const std::string& key, const Index& value);
	void remove( DatabaseTransactionInterface* transaction, const std::string& key);
	void storeImm( const std::string& key, const Index& value);

private:
	char m_prefix;
	DatabaseInterface* m_database;
};

class DatabaseAdapter_TermType
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::TermTypePrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};

	explicit DatabaseAdapter_TermType( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};

class DatabaseAdapter_TermValue
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::TermValuePrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};
	explicit DatabaseAdapter_TermValue( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};

class DatabaseAdapter_DocId
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::DocIdPrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};
	explicit DatabaseAdapter_DocId( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};

class DatabaseAdapter_Variable
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::VariablePrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};
	explicit DatabaseAdapter_Variable( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};

class DatabaseAdapter_AttributeKey
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::AttributeKeyPrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};
	explicit DatabaseAdapter_AttributeKey( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};

class DatabaseAdapter_UserName
	:public DatabaseAdapter_StringIndex
{
public:
	enum {KeyPrefix=DatabaseKey::UserNamePrefix};
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
		Cursor( DatabaseInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( (char)KeyPrefix, database_){}
	};
	explicit DatabaseAdapter_UserName( DatabaseInterface* database_)
		:DatabaseAdapter_StringIndex((char)KeyPrefix, database_){}
};


class DatabaseAdapter_DataBlock
{
public:
	DatabaseAdapter_DataBlock( DatabaseInterface* database_, char prefix_, const BlockKey& domainKey_)
		:m_database(database_),m_dbkey(prefix_,domainKey_),m_domainKeySize(0)
	{
		m_domainKeySize = m_dbkey.size();
	}

	bool load( const Index& docno, DataBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const DataBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& elemno);
	void removeSubTree( DatabaseTransactionInterface* transaction);

protected:
	enum {KeyPrefix=DatabaseKey::PosinfoBlockPrefix};
	DatabaseInterface* m_database;
	DatabaseKey m_dbkey;
	std::size_t m_domainKeySize;
};


class DatabaseAdapter_DataBlock_Cursor
	:public DatabaseAdapter_DataBlock
{
public:
	DatabaseAdapter_DataBlock_Cursor( DatabaseInterface* database_, char prefix_, const BlockKey& domainKey_)
		:DatabaseAdapter_DataBlock( database_,prefix_,domainKey_),m_cursor(database_->createCursor(true))
	{
		m_domainKeySize = m_dbkey.size();
	}

	bool loadUpperBound( const Index& elemno, DataBlock& blk);
	bool loadFirst( DataBlock& blk);
	bool loadNext( DataBlock& blk);
	bool loadLast( DataBlock& blk);

private:
	bool getBlock( const DatabaseCursorInterface::Slice& key, DataBlock& blk);

protected:
	Reference<DatabaseCursorInterface> m_cursor;
};


class DatabaseAdapter_ForwardIndex_Cursor
	:protected DatabaseAdapter_DataBlock_Cursor
{
public:
	DatabaseAdapter_ForwardIndex_Cursor( DatabaseInterface* database_, const Index& typeno_, const Index& docno_)
		:DatabaseAdapter_DataBlock_Cursor( database_,(char)KeyPrefix, BlockKey(typeno_,docno_)){}

	bool load( const Index& posno, ForwardIndexBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const ForwardIndexBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& posno);
	void removeSubTree( DatabaseTransactionInterface* transaction);

	bool loadUpperBound( const Index& posno, ForwardIndexBlock& blk);
	bool loadFirst( ForwardIndexBlock& blk);
	bool loadNext( ForwardIndexBlock& blk);
	bool loadLast( ForwardIndexBlock& blk);

private:
	enum {KeyPrefix=DatabaseKey::ForwardIndexPrefix};
};


class DatabaseAdapter_PosinfoBlock_Cursor
	:public DatabaseAdapter_DataBlock_Cursor
{
public:
	DatabaseAdapter_PosinfoBlock_Cursor( DatabaseInterface* database_, const Index& typeno_, const Index& termno_)
		:DatabaseAdapter_DataBlock_Cursor( database_,(char)KeyPrefix, BlockKey(typeno_,termno_)){}

	bool loadUpperBound( const Index& docno, PosinfoBlock& blk);
	bool loadFirst( PosinfoBlock& blk);
	bool loadNext( PosinfoBlock& blk);
	bool loadLast( PosinfoBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const PosinfoBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& docno);

private:
	enum {KeyPrefix=DatabaseKey::PosinfoBlockPrefix};
};


class DatabaseAdapter_InverseTerm
	:protected DatabaseAdapter_DataBlock
{
public:
	DatabaseAdapter_InverseTerm( DatabaseInterface* database_)
		:DatabaseAdapter_DataBlock( database_,(char)KeyPrefix, BlockKey()){}

	bool load( const Index& docno, InvTermBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const InvTermBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& docno);
	void removeSubTree( DatabaseTransactionInterface* transaction);

private:
	enum {KeyPrefix=DatabaseKey::InverseTermPrefix};
};


class DatabaseAdapter_BooleanBlock_Cursor
	:public DatabaseAdapter_DataBlock_Cursor
{
public:
	DatabaseAdapter_BooleanBlock_Cursor( DatabaseInterface* database_, char prefix, const BlockKey& domainKey_)
		:DatabaseAdapter_DataBlock_Cursor( database_, prefix, domainKey_){}

	bool loadUpperBound( const Index& docno, BooleanBlock& blk);
	bool loadFirst( BooleanBlock& blk);
	bool loadNext( BooleanBlock& blk);
	bool loadLast( BooleanBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const BooleanBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& docno);
};


class DatabaseAdapter_UserAclBlock_Cursor
	:public DatabaseAdapter_BooleanBlock_Cursor
{
public:
	DatabaseAdapter_UserAclBlock_Cursor( DatabaseInterface* database_, const Index& docno_)
		:DatabaseAdapter_BooleanBlock_Cursor( database_, (char)KeyPrefix, BlockKey(docno_)){}

private:
	enum {KeyPrefix=DatabaseKey::UserAclBlockPrefix};
};


class DatabaseAdapter_AclBlock_Cursor
	:public DatabaseAdapter_BooleanBlock_Cursor
{
public:
	DatabaseAdapter_AclBlock_Cursor( DatabaseInterface* database_, const Index& userno_)
		:DatabaseAdapter_BooleanBlock_Cursor( database_, (char)KeyPrefix, BlockKey(userno_)){}
	
private:
	enum {KeyPrefix=DatabaseKey::AclBlockPrefix};
};


class DatabaseAdapter_DocListBlock_Cursor
	:public DatabaseAdapter_BooleanBlock_Cursor
{
public:
	DatabaseAdapter_DocListBlock_Cursor( DatabaseInterface* database_, const Index& typeno_, const Index& termno_)
		:DatabaseAdapter_BooleanBlock_Cursor( database_, (char)KeyPrefix, BlockKey(typeno_,termno_)){}
private:
	enum {KeyPrefix=DatabaseKey::DocListBlockPrefix};
};


class DatabaseAdapter_DocMetaData
{
public:
	explicit DatabaseAdapter_DocMetaData( const DatabaseInterface* database_, const MetaDataDescription* descr_)
		:m_database( database_),m_descr(descr_){}

	MetaDataBlock* loadPtr( const Index& blockno);
	bool load( const Index& blockno, MetaDataBlock& blk);
	bool loadFirst( MetaDataBlock& blk);
	bool loadNext( MetaDataBlock& blk);
	void store( DatabaseTransactionInterface* transaction, const MetaDataBlock& blk);
	void remove( DatabaseTransactionInterface* transaction, const Index& blockno);

private:
	bool getBlock( const DatabaseCursorInterface::Slice& key, MetaDataBlock& blk);

private:
	enum {KeyPrefix=DatabaseKey::DocMetaDataPrefix};
	const DatabaseInterface* m_database;
	Reference<DatabaseCursorInterface> m_cursor;
	const MetaDataDescription* m_descr;
};


class DatabaseAdapter_DocAttribute
{
public:
	static bool load( const DatabaseInterface* database, const Index& docno, const Index& attrno, std::string& value);
	static void store( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno, const char* value);
	static void remove( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno);
	static void removeAll( DatabaseTransactionInterface* transaction, const Index& docno);

private:
	enum {KeyPrefix=DatabaseKey::DocAttributePrefix};
};

class DatabaseAdapter_DocFrequency
{
public:
	class Cursor
	{
	public:
		Cursor( DatabaseInterface* database_)
			:m_cursor( database_->createCursor(false)){}

		bool loadFirst( Index& typeno, Index& termno, Index& df);
		bool loadNext( Index& typeno, Index& termno, Index& df);

	private:
		bool getData( const DatabaseCursorInterface::Slice& key, Index& typeno, Index& termno, Index& df);

	private:
		Reference<DatabaseCursorInterface> m_cursor;
	};
public:
	static Index get( const DatabaseInterface* database, const Index& typeno, const Index& termno);
	static bool load( const DatabaseInterface* database, const Index& typeno, const Index& termno, Index& df);
	static void store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& df);
	static void remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno);
	static void storeImm( DatabaseInterface* database, const Index& typeno, const Index& termno, const Index& df);
private:
	enum {KeyPrefix=DatabaseKey::DocFrequencyPrefix};
};

class DatabaseAdapter_MetaDataDescr
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


