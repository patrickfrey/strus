/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Database abstraction (wrapper) for the strus storage
#ifndef _STRUS_DATABASE_ADAPTER_HPP_INCLUDED
#define _STRUS_DATABASE_ADAPTER_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/databaseOptions.hpp"
#include "strus/reference.hpp"
#include "databaseKey.hpp"
#include "dataBlock.hpp"
#include "posinfoBlock.hpp"
#include "structBlock.hpp"
#include "booleanBlock.hpp"
#include "invTermBlock.hpp"
#include "forwardIndexBlock.hpp"
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
class InvTermBlock;
/// \brief Forward declaration
class DataBlock;

struct DatabaseAdapter_StringIndex
{
	class Cursor
	{
	public:
		Cursor( char prefix_, const DatabaseClientInterface* database_);
		Cursor( const Cursor& o)
			:m_cursor(o.m_cursor),m_prefix(o.m_prefix){}

		bool skip( const std::string& key, std::string& keyfound, Index& value);
		bool skipPrefix( const std::string& key, std::string& keyfound, Index& value);
		bool loadFirst( std::string& key, Index& value);
		bool loadNext( std::string& key, Index& value);
		bool loadNextPrefix( const std::string& key, std::string& keyfound, Index& value);

	private:
		bool getData( const DatabaseCursorInterface::Slice& dbkey, std::string& key, Index& value);
	
	private:
		Reference<DatabaseCursorInterface> m_cursor;
		char m_prefix;
	};

	class Reader
	{
	public:
		Reader( char prefix_, const DatabaseClientInterface* database_)
			:m_prefix(prefix_),m_database(database_){}
		Reader( const Reader& o)
			:m_prefix(o.m_prefix),m_database(o.m_database){}

		Index get( const std::string& key) const;
		bool load( const std::string& key, Index& value) const;
	
	private:
		char m_prefix;
		const DatabaseClientInterface* m_database;
	};

	class Writer
	{
	public:
		Writer( char prefix_, DatabaseClientInterface* database_)
			:m_prefix(prefix_),m_database(database_){}
		Writer( const Writer& o)
			:m_prefix(o.m_prefix),m_database(o.m_database){}

		void store( DatabaseTransactionInterface* transaction, const std::string& key, const Index& value);
		void remove( DatabaseTransactionInterface* transaction, const std::string& key);
		void storeImm( const std::string& key, const Index& value);
	
	private:
		char m_prefix;
		DatabaseClientInterface* m_database;
	};

	class WriteCursor
		:public Writer
		,public Cursor
	{
	public:
		WriteCursor( char prefix_, DatabaseClientInterface* database_)
			:Writer(prefix_,database_)
			,Cursor(prefix_,database_){}
		WriteCursor( const WriteCursor& o)
			:Writer(o),Cursor(o){}
	};

	class ReadWriter
		:public Reader
		,public Writer
	{
	public:
		ReadWriter( char prefix_, DatabaseClientInterface* database_)
			:Reader(prefix_,database_)
			,Writer(prefix_,database_){}
		ReadWriter( const ReadWriter& o)
			:Reader(o),Writer(o){}
	};
};

class DatabaseAdapter_IndexString
{
public:
	class Reader
	{
	public:
		Reader( char prefix_, const DatabaseClientInterface* database_)
			:m_prefix(prefix_),m_database(database_){}
		Reader( const Reader& o)
			:m_prefix(o.m_prefix),m_database(o.m_database){}

		bool defined()		{return m_prefix != 0;}

		bool load( const Index& key, std::string& value) const;
	
	private:
		char m_prefix;
		const DatabaseClientInterface* m_database;
	};

	class Writer
	{
	public:
		Writer( char prefix_, DatabaseClientInterface* database_)
			:m_prefix(prefix_),m_database(database_){}
		Writer( const Writer& o)
			:m_prefix(o.m_prefix),m_database(o.m_database){}

		bool defined()		{return m_prefix != 0;}

		void store( DatabaseTransactionInterface* transaction, const Index& key, const char* value);
		void remove( DatabaseTransactionInterface* transaction, const Index& key);
		void storeImm( const Index& key, const std::string& value);
	
	private:
		char m_prefix;
		DatabaseClientInterface* m_database;
	};

	class ReadWriter
		:public Reader
		,public Writer
	{
	public:
		ReadWriter( char prefix_, DatabaseClientInterface* database_)
			:Reader(prefix_,database_)
			,Writer(prefix_,database_){}
		ReadWriter( const ReadWriter& o)
			:Reader(o),Writer(o){}

		bool defined()		{return Reader::defined();}
	};
};


template <char KeyPrefix>
struct DatabaseAdapter_TypedStringIndex
{
public:
	class Cursor
		:public DatabaseAdapter_StringIndex::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_)
			:DatabaseAdapter_StringIndex::Cursor( KeyPrefix, database_){}
		Cursor( const Cursor& o)
			:Cursor(o){}
	};
	class Reader
		:public DatabaseAdapter_StringIndex::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_)
			:DatabaseAdapter_StringIndex::Reader( KeyPrefix, database_){}
		Reader( const Reader& o)
			:Reader(o){}
	};
	class Writer
		:public DatabaseAdapter_StringIndex::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_)
			:DatabaseAdapter_StringIndex::Writer( KeyPrefix, database_){}
		Writer( const Writer& o)
			:Writer(o){}
	};
	class ReadWriter
		:public Reader
		,public Writer
	{
	public:
		ReadWriter( DatabaseClientInterface* database_)
			:Reader( database_),Writer( database_){}
		ReadWriter( const Writer& o)
			:Reader(o),Writer(o){}
	};
};

template <char KeyPrefix>
struct DatabaseAdapter_TypedIndexString
{
public:
	class Reader
		:public DatabaseAdapter_IndexString::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_)
			:DatabaseAdapter_IndexString::Reader( KeyPrefix, database_){}
		Reader( const Reader& o)
			:DatabaseAdapter_IndexString::Reader(o){}
	};
	class Writer
		:public DatabaseAdapter_IndexString::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_)
			:DatabaseAdapter_IndexString::Writer( KeyPrefix, database_){}
		Writer( const Writer& o)
			:DatabaseAdapter_IndexString::Writer(o){}
	};
	class ReadWriter
		:public Reader
		,public Writer
	{
	public:
		ReadWriter( DatabaseClientInterface* database_)
			:Reader( database_),Writer( database_){}
		ReadWriter( const ReadWriter& o)
			:Reader(o),Writer(o){}
	};
};

struct DatabaseAdapter_TermType
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::TermTypePrefix>
{};

struct DatabaseAdapter_TermValue
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::TermValuePrefix>
{};

struct DatabaseAdapter_StructType
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::StructTypePrefix>
{};

struct DatabaseAdapter_DocId
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::DocIdPrefix>
{};

struct DatabaseAdapter_Variable
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::VariablePrefix>
{};

struct DatabaseAdapter_AttributeKey
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::AttributeKeyPrefix>
{};

struct DatabaseAdapter_UserName
	:public DatabaseAdapter_TypedStringIndex<DatabaseKey::UserNamePrefix>
{};


struct DatabaseAdapter_DataBlock
{
	class Base
	{
	public:
		Base( char prefix_, const BlockKey& domainKey_)
			:m_dbkey(prefix_,domainKey_),m_domainKeySize(0)
		{
			m_domainKeySize = m_dbkey.size();
		}
		Base( const Base& o)
			:m_dbkey(o.m_dbkey),m_domainKeySize(o.m_domainKeySize){}

	protected:
		DatabaseKey m_dbkey;
		std::size_t m_domainKeySize;
	};

	class Reader
		:public Base
	{
	public:
		Reader( char prefix_, const DatabaseClientInterface* database_, const BlockKey& domainKey_, bool useCache_)
			:Base(prefix_,domainKey_),m_database(database_),m_useCache(useCache_){}
		Reader( const Reader& o)
			:Base(o),m_database(o.m_database),m_useCache(o.m_useCache){}

		bool load( const Index& docno, DataBlock& blk);

	private:
		const DatabaseClientInterface* m_database;
		bool m_useCache;
	};

	class Writer
		:public Base
	{
	public:
		Writer( char prefix_, DatabaseClientInterface* database_, const BlockKey& domainKey_)
			:Base(prefix_,domainKey_),m_database(database_){}
		Writer( const Writer& o)
			:Base(o),m_database(o.m_database){}

		void store( DatabaseTransactionInterface* transaction, const DataBlock& blk);
		void remove( DatabaseTransactionInterface* transaction, const Index& elemno);
		void removeSubTree( DatabaseTransactionInterface* transaction);

	private:
		DatabaseClientInterface* m_database;
	};

	class Cursor
		:public Base
	{
	public:
		Cursor( char prefix_, const DatabaseClientInterface* database_, const BlockKey& domainKey_, bool useCache_);
		Cursor( const Cursor& o)
			:Base(o),m_cursor(o.m_cursor){}

		bool loadUpperBound( const Index& elemno, DataBlock& blk);
		bool loadFirst( DataBlock& blk);
		bool loadNext( DataBlock& blk);
		bool loadLast( DataBlock& blk);

	private:
		bool getBlock( const DatabaseCursorInterface::Slice& key, DataBlock& blk);

	protected:
		Reference<DatabaseCursorInterface> m_cursor;
	};
};


template <char KeyPrefix,class DataBlockType,bool UseCache>
struct DatabaseAdapter_TypedDataBlock
{
	class Reader
		:public DatabaseAdapter_DataBlock::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_, const BlockKey& domainKey_,bool useCache_=UseCache)
			:DatabaseAdapter_DataBlock::Reader(KeyPrefix,database_,domainKey_,useCache_){}
		Reader( const Reader& o)
			:DatabaseAdapter_DataBlock::Reader(o){}

		bool load( const Index& elemno, DataBlockType& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Reader::load( elemno, blk_)) return false;
			blk.swap( blk_);
			return true;
		}
	};

	class Writer
		:public DatabaseAdapter_DataBlock::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_, const BlockKey& domainKey_)
			:DatabaseAdapter_DataBlock::Writer(KeyPrefix,database_,domainKey_){}
		Writer( const Writer& o)
			:DatabaseAdapter_DataBlock::Writer(o){}

		void store( DatabaseTransactionInterface* transaction, const DataBlockType& blk)
		{
			DatabaseAdapter_DataBlock::Writer::store( transaction, blk);
		}
	};

	class Cursor
		:public DatabaseAdapter_DataBlock::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_, const BlockKey& domainKey_,bool useCache_=UseCache)
			:DatabaseAdapter_DataBlock::Cursor(KeyPrefix,database_,domainKey_,useCache_){}
		Cursor( const Cursor& o)
			:DatabaseAdapter_DataBlock::Cursor(o){}

		bool loadUpperBound( const Index& elemno, DataBlockType& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadUpperBound( elemno, blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadFirst( DataBlockType& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadFirst( blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadNext( DataBlockType& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadNext( blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadLast( DataBlockType& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadLast( blk_)) return false;
			blk.swap( blk_);
			return true;
		}
	};
};

struct DatabaseAdapter_ForwardIndex
{
	typedef DatabaseAdapter_TypedDataBlock<
			DatabaseKey::ForwardIndexPrefix, ForwardIndexBlock, false> Parent;

	class Reader
		:public Parent::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_,
			const Index& typeno_, const Index& docno_)
			:Parent::Reader( database_, BlockKey(typeno_,docno_)){}
		Reader( const Reader& o)
			:Parent::Reader(o){}
	};
	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_,
			const Index& typeno_, const Index& docno_)
			:Parent::Writer( database_, BlockKey(typeno_,docno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_,
			const Index& typeno_, const Index& docno_)
			:Parent::Cursor( database_, BlockKey(typeno_,docno_)){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	static bool exists( const DatabaseClientInterface* database_, const Index& typeno_);
};


struct DatabaseAdapter_PosinfoBlock
{
	typedef DatabaseAdapter_TypedDataBlock<
			DatabaseKey::PosinfoBlockPrefix, PosinfoBlock, true> Parent;

	class Reader
		:public Parent::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_,
			const Index& typeno_, const Index& termno_)
			:Parent::Reader( database_, BlockKey(typeno_,termno_)){}
		Reader( const Reader& o)
			:Parent::Reader(o){}
	};
	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_,
			const Index& typeno_, const Index& termno_)
			:Parent::Writer( database_, BlockKey(typeno_,termno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_,
			const Index& typeno_, const Index& termno_)
			:Parent::Cursor( database_, BlockKey(typeno_,termno_)){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	class WriteCursor
		:public Cursor
		,public Writer
	{
	public:
		WriteCursor( DatabaseClientInterface* database_, 
				const Index& typeno_, const Index& termno_)
			:Cursor(database_,typeno_,termno_)
			,Writer(database_,typeno_,termno_){}
		WriteCursor( const WriteCursor& o)
			:Cursor(o),Writer(o){}
	};
};

struct DatabaseAdapter_StructBlock
{
	typedef DatabaseAdapter_TypedDataBlock<
			DatabaseKey::StructBlockPrefix, StructBlock, true> Parent;

	class Reader
		:public Parent::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_,
			const Index& structno_)
			:Parent::Reader( database_, BlockKey(structno_)){}
		Reader( const Reader& o)
			:Parent::Reader(o){}
	};
	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_,
			const Index& structno_)
			:Parent::Writer( database_, BlockKey(structno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_,
			const Index& structno_)
			:Parent::Cursor( database_, BlockKey(structno_)){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	class WriteCursor
		:public Cursor
		,public Writer
	{
	public:
		WriteCursor( DatabaseClientInterface* database_, 
				const Index& structno_)
			:Cursor(database_,structno_)
			,Writer(database_,structno_){}
		WriteCursor( const WriteCursor& o)
			:Cursor(o),Writer(o){}
	};
};


struct DatabaseAdapter_InverseTerm
{
	typedef DatabaseAdapter_TypedDataBlock<
			DatabaseKey::InverseTermPrefix, InvTermBlock, false> Parent;

	class Reader
		:public Parent::Reader
	{
	public:
		Reader( const DatabaseClientInterface* database_)
			:Parent::Reader( database_, BlockKey()){}
		Reader( const Reader& o)
			:Parent::Reader(o){}
	};
	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_)
			:Parent::Writer( database_, BlockKey()){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};

	class ReadWriter
		:public Reader
		,public Writer
	{
	public:
		ReadWriter( DatabaseClientInterface* database_)
			:Reader(database_)
			,Writer(database_){}
		ReadWriter( const ReadWriter& o)
			:Reader(o),Writer(o){}
	};

	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_)
			:Parent::Cursor( database_, BlockKey()){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};
};


struct DatabaseAdapter_BooleanBlock
{
	class Writer
		:public DatabaseAdapter_DataBlock::Writer
	{
	public:
		Writer( char prefix_, DatabaseClientInterface* database_, const BlockKey& domainKey_)
			:DatabaseAdapter_DataBlock::Writer(prefix_,database_,domainKey_){}
		Writer( const Writer& o)
			:DatabaseAdapter_DataBlock::Writer(o){}

		void store( DatabaseTransactionInterface* transaction, const BooleanBlock& blk)
		{
			DatabaseAdapter_DataBlock::Writer::store( transaction, blk);
		}
	};

	class Cursor
		:public DatabaseAdapter_DataBlock::Cursor
	{
	public:
		Cursor( char prefix_, const DatabaseClientInterface* database_, const BlockKey& domainKey_, bool useCache_)
			:DatabaseAdapter_DataBlock::Cursor(prefix_,database_,domainKey_,useCache_){}
		Cursor( const Cursor& o)
			:DatabaseAdapter_DataBlock::Cursor(o){}

		bool loadUpperBound( const Index& elemno, BooleanBlock& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadUpperBound( elemno, blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadFirst( BooleanBlock& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadFirst( blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadNext( BooleanBlock& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadNext( blk_)) return false;
			blk.swap( blk_);
			return true;
		}

		bool loadLast( BooleanBlock& blk)
		{
			DataBlock blk_;
			if (!DatabaseAdapter_DataBlock::Cursor::loadLast( blk_)) return false;
			blk.swap( blk_);
			return true;
		}
	};

	class WriteCursor
		:public Cursor
		,public Writer
	{
	public:
		WriteCursor( char prefix_, DatabaseClientInterface* database_, const BlockKey& domainKey_)
			:Cursor(prefix_,database_,domainKey_,false)
			,Writer(prefix_,database_,domainKey_){}
		WriteCursor( const WriteCursor& o)
			:Cursor(o),Writer(o){}
	};
};


struct DatabaseAdapter_UserAclBlock
{
	enum {KeyPrefix=DatabaseKey::UserAclBlockPrefix};
	typedef DatabaseAdapter_BooleanBlock Parent;

	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_, const Index& userno_)
			:Parent::Writer( (char)KeyPrefix, database_, BlockKey(userno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_, const Index& userno_, bool useCache_)
			:Parent::Cursor( (char)KeyPrefix, database_, BlockKey(userno_), useCache_){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	class WriteCursor
		:public Parent::WriteCursor
	{
	public:
		WriteCursor( DatabaseClientInterface* database_, const Index& userno_)
			:Parent::WriteCursor((char)KeyPrefix, database_,BlockKey(userno_)){}
		WriteCursor( const WriteCursor& o)
			:Parent::WriteCursor(o){}
	};
};


struct DatabaseAdapter_AclBlock
{
	enum {KeyPrefix=DatabaseKey::AclBlockPrefix};
	typedef DatabaseAdapter_BooleanBlock Parent;

	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_, const Index& docno_)
			:Parent::Writer( (char)KeyPrefix, database_, BlockKey(docno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_, const Index& docno_, bool useCache_)
			:Parent::Cursor( (char)KeyPrefix, database_, BlockKey(docno_), useCache_){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	class WriteCursor
		:public Parent::WriteCursor
	{
	public:
		WriteCursor( DatabaseClientInterface* database_, const Index& docno_)
			:Parent::WriteCursor((char)KeyPrefix, database_,BlockKey(docno_)){}
		WriteCursor( const WriteCursor& o)
			:Parent::WriteCursor(o){}
	};
};


struct DatabaseAdapter_DocListBlock
{
	enum {KeyPrefix=DatabaseKey::DocListBlockPrefix};
	typedef DatabaseAdapter_BooleanBlock Parent;

	class Writer
		:public Parent::Writer
	{
	public:
		Writer( DatabaseClientInterface* database_,
			const Index& typeno_, const Index& termno_)
			:Parent::Writer( (char)KeyPrefix, database_, BlockKey(typeno_,termno_)){}
		Writer( const Writer& o)
			:Parent::Writer(o){}
	};
	class Cursor
		:public Parent::Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_,
			const Index& typeno_, const Index& termno_, bool useCache_)
			:Parent::Cursor( (char)KeyPrefix, database_, BlockKey(typeno_,termno_), useCache_){}
		Cursor( const Cursor& o)
			:Parent::Cursor(o){}
	};

	class WriteCursor
		:public Parent::WriteCursor
	{
	public:
		WriteCursor( DatabaseClientInterface* database_, const Index& typeno_, const Index& termno_)
			:Parent::WriteCursor((char)KeyPrefix, database_,BlockKey(typeno_,termno_)){}
		WriteCursor( const WriteCursor& o)
			:Parent::WriteCursor(o){}
	};
};


class DatabaseAdapter_DocMetaData
{
public:
	explicit DatabaseAdapter_DocMetaData( const DatabaseClientInterface* database_, const MetaDataDescription* descr_)
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
	const DatabaseClientInterface* m_database;
	Reference<DatabaseCursorInterface> m_cursor;
	const MetaDataDescription* m_descr;
};


class DatabaseAdapter_DocAttribute
{
public:
	static bool load( const DatabaseClientInterface* database, const Index& docno, const Index& attrno, std::string& value);
	static void store( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno, const char* value);
	static void remove( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno);
	static void removeAll( DatabaseTransactionInterface* transaction, const Index& docno);

private:
	enum {KeyPrefix=DatabaseKey::DocAttributePrefix};
};


class DatabaseAdapter_TermTypeInv
	:public DatabaseAdapter_TypedIndexString<DatabaseKey::TermTypeInvPrefix>
{};

class DatabaseAdapter_TermValueInv
	:public DatabaseAdapter_TypedIndexString<DatabaseKey::TermValueInvPrefix>
{};

class DatabaseAdapter_StructTypeInv
	:public DatabaseAdapter_TypedIndexString<DatabaseKey::StructTypeInvPrefix>
{};


class DatabaseAdapter_DocFrequency
{
public:
	class Cursor
	{
	public:
		Cursor( const DatabaseClientInterface* database_);

		bool loadFirst( Index& typeno, Index& termno, Index& df);
		bool loadNext( Index& typeno, Index& termno, Index& df);

		bool loadFirst_typeno( const Index& typeno, Index& termno, Index& df);
		bool loadNext_typeno( const Index& typeno, Index& termno, Index& df);

	private:
		bool getData( const DatabaseCursorInterface::Slice& key, Index& typeno, Index& termno, Index& df);

	private:
		Reference<DatabaseCursorInterface> m_cursor;
	};
public:
	static Index get( const DatabaseClientInterface* database, const Index& typeno, const Index& termno);
	static bool load( const DatabaseClientInterface* database, const Index& typeno, const Index& termno, Index& df);
	static void store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& df);
	static void remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno);
	static void storeImm( DatabaseClientInterface* database, const Index& typeno, const Index& termno, const Index& df);
private:
	enum {KeyPrefix=DatabaseKey::DocFrequencyPrefix};
};

class DatabaseAdapter_MetaDataDescr
{
public:
	static bool load( const DatabaseClientInterface* database, std::string& descr);
	static void store( DatabaseTransactionInterface* transaction, const std::string& descr);
	static void storeImm( DatabaseClientInterface* database, const std::string& descr);
private:
	enum {KeyPrefix=DatabaseKey::MetaDataDescrPrefix};
};

}//namespace
#endif


