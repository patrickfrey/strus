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
#include "databaseAdapter.hpp"
#include "indexPacker.hpp"
#include "metaDataBlock.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"

using namespace strus;

bool DatabaseAdapter_StringIndex::Cursor::getData( const DatabaseCursorInterface::Slice& dbkey, std::string& key, Index& value)
{
	if (!dbkey.defined()) return false;
	key = std::string( dbkey.ptr()+1, dbkey.size()-1);
	DatabaseCursorInterface::Slice blkslice = m_cursor->value();
	char const* vi = blkslice.ptr();
	char const* ve = vi + blkslice.size();
	value = unpackIndex( vi, ve);
	return true;
}

bool DatabaseAdapter_StringIndex::Cursor::loadFirst( std::string& key, Index& value)
{
	DatabaseCursorInterface::Slice dbkey( m_cursor->seekFirst( &m_prefix, 1));
	return getData( dbkey, key, value);
}

bool DatabaseAdapter_StringIndex::Cursor::loadNext( std::string& key, Index& value)
{
	DatabaseCursorInterface::Slice dbkey( m_cursor->seekNext());
	return getData( dbkey, key, value);
}

Index DatabaseAdapter_StringIndex::Reader::get( const std::string& key) const
{
	Index rt;
	if (!DatabaseAdapter_StringIndex::Reader::load( key, rt)) return 0;
	return rt;
}

bool DatabaseAdapter_StringIndex::Reader::load( const std::string& key, Index& value) const
{
	std::string keystr;
	keystr.push_back( m_prefix);
	keystr.append( key);
	std::string valuestr;
	value = 0;
	if (!m_database->readValue( keystr.c_str(), keystr.size(), valuestr, DatabaseOptions())) return false;

	char const* cc = valuestr.c_str();
	value = unpackIndex( cc, cc + valuestr.size());
	return true;
}

void DatabaseAdapter_StringIndex::Writer::store( DatabaseTransactionInterface* transaction, const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( m_prefix);
	keystr.append( key);
	std::string valuestr;
	packIndex( valuestr, value);
	transaction->write( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}

void DatabaseAdapter_StringIndex::Writer::remove( DatabaseTransactionInterface* transaction, const std::string& key)
{
	std::string keystr;
	keystr.push_back( m_prefix);
	keystr.append( key);
	transaction->remove( keystr.c_str(), keystr.size());
}

void DatabaseAdapter_StringIndex::Writer::storeImm( const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( m_prefix);
	keystr.append( key);
	std::string valuestr;
	packIndex( valuestr, value);
	m_database->writeImm( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}


bool DatabaseAdapter_DataBlock::Reader::load( const Index& elemno, DataBlock& blk)
{
	m_dbkey.resize( m_domainKeySize);
	std::string blkstr;
	if (!m_database->readValue( m_dbkey.ptr(), m_dbkey.size(), blkstr, DatabaseOptions())) return false;
	blk.init( elemno, blkstr.c_str(), blkstr.size(), blkstr.size());
	return true;
}

void DatabaseAdapter_DataBlock::Writer::store( DatabaseTransactionInterface* transaction, const DataBlock& blk)
{
	m_dbkey.resize( m_domainKeySize);
	m_dbkey.addElem( blk.id());
	transaction->write( m_dbkey.ptr(), m_dbkey.size(), blk.charptr(), blk.size());
}

void DatabaseAdapter_DataBlock::Writer::remove( DatabaseTransactionInterface* transaction, const Index& elemno)
{
	m_dbkey.resize( m_domainKeySize);
	m_dbkey.addElem( elemno);
	transaction->remove( m_dbkey.ptr(), m_dbkey.size());
}

void DatabaseAdapter_DataBlock::Writer::removeSubTree( DatabaseTransactionInterface* transaction)
{
	m_dbkey.resize( m_domainKeySize);
	transaction->removeSubTree( m_dbkey.ptr(), m_dbkey.size());
}

bool DatabaseAdapter_DataBlock::Cursor::getBlock( const DatabaseCursorInterface::Slice& key, DataBlock& blk)
{
	if (!key.defined()) return false;
	char const* ki = key.ptr()+m_domainKeySize;
	char const* ke = ki + key.size()-m_domainKeySize;
	Index elemno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = m_cursor->value();
	blk.init( elemno, blkslice.ptr(), blkslice.size());
	return true;
}

bool DatabaseAdapter_DataBlock::Cursor::loadUpperBound( const Index& elemno, DataBlock& blk)
{
	m_dbkey.resize( m_domainKeySize);
	m_dbkey.addElem( elemno);
	DatabaseCursorInterface::Slice key = m_cursor->seekUpperBound( m_dbkey.ptr(), m_dbkey.size(), m_domainKeySize);
	return getBlock( key, blk);
}

bool DatabaseAdapter_DataBlock::Cursor::loadFirst( DataBlock& blk)
{
	m_dbkey.resize( m_domainKeySize);
	DatabaseCursorInterface::Slice key = m_cursor->seekFirst( m_dbkey.ptr(), m_dbkey.size());
	return getBlock( key, blk);
}

bool DatabaseAdapter_DataBlock::Cursor::loadNext( DataBlock& blk)
{
	DatabaseCursorInterface::Slice key = m_cursor->seekNext();
	return getBlock( key, blk);
}

bool DatabaseAdapter_DataBlock::Cursor::loadLast( DataBlock& blk)
{
	m_dbkey.resize( m_domainKeySize);
	DatabaseCursorInterface::Slice key = m_cursor->seekLast( m_dbkey.ptr(), m_dbkey.size());
	return getBlock( key, blk);
}


bool DatabaseAdapter_DocMetaData::getBlock( const DatabaseCursorInterface::Slice& key, MetaDataBlock& blk)
{
	if (!key.defined()) return false;
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	Index blockno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = m_cursor->value();
	blk.init( m_descr, blockno, blkslice.ptr(), blkslice.size());
	return true;
}

bool DatabaseAdapter_DocMetaData::load( const Index& blockno, MetaDataBlock& blk)
{
	std::string blkstr;
	DatabaseKey dbkey( KeyPrefix, blockno);
	if (!m_database->readValue( dbkey.ptr(), dbkey.size(), blkstr, DatabaseOptions())) return false;
	blk.init( m_descr, blockno, blkstr.c_str(), blkstr.size());
	return true;
}

MetaDataBlock* DatabaseAdapter_DocMetaData::loadPtr( const Index& blockno)
{
	std::string blkstr;
	DatabaseKey dbkey( KeyPrefix, blockno);
	if (!m_database->readValue( dbkey.ptr(), dbkey.size(), blkstr, DatabaseOptions())) return 0;
	return new MetaDataBlock( m_descr, blockno, blkstr.c_str(), blkstr.size());
}

bool DatabaseAdapter_DocMetaData::loadFirst( MetaDataBlock& blk)
{
	if (!m_cursor.get())
	{
		m_cursor.reset( m_database->createCursor( DatabaseOptions()));
	}
	DatabaseKey dbkey( KeyPrefix);
	DatabaseCursorInterface::Slice key = m_cursor->seekFirst( dbkey.ptr(), dbkey.size());
	return getBlock( key, blk);
}

bool DatabaseAdapter_DocMetaData::loadNext( MetaDataBlock& blk)
{
	if (!m_cursor.get()) return false;
	DatabaseCursorInterface::Slice key = m_cursor->seekNext();
	return getBlock( key, blk);
}

void DatabaseAdapter_DocMetaData::store( DatabaseTransactionInterface* transaction, const MetaDataBlock& blk)
{
	DatabaseKey dbkey( KeyPrefix, blk.blockno());
	transaction->write( dbkey.ptr(), dbkey.size(), blk.charptr(), blk.bytesize());
}

void DatabaseAdapter_DocMetaData::remove( DatabaseTransactionInterface* transaction, const Index& blockno)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	transaction->remove( dbkey.ptr(), dbkey.size());
}


bool DatabaseAdapter_DocAttribute::load( const DatabaseClientInterface* database, const Index& docno, const Index& attrno, std::string& value)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return database->readValue( dbkey.ptr(), dbkey.size(), value, DatabaseOptions().useCache());
}

void DatabaseAdapter_DocAttribute::store( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno, const char* value)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return transaction->write( dbkey.ptr(), dbkey.size(), value, std::strlen(value));
}

void DatabaseAdapter_DocAttribute::remove( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return transaction->remove( dbkey.ptr(), dbkey.size());
}

void DatabaseAdapter_DocAttribute::removeAll( DatabaseTransactionInterface* transaction, const Index& docno)
{
	DatabaseKey dbkey( KeyPrefix, docno);
	return transaction->removeSubTree( dbkey.ptr(), dbkey.size());
}


bool DatabaseAdapter_DocFrequency::Cursor::getData( const DatabaseCursorInterface::Slice& key, Index& typeno, Index& termno, Index& df)
{
	if (!key.defined()) return false;
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	typeno = unpackIndex( ki, ke);
	termno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = m_cursor->value();
	char const* vi = blkslice.ptr();
	char const* ve = vi + blkslice.size();
	df = unpackIndex( vi, ve);
	return true;
}

bool DatabaseAdapter_DocFrequency::Cursor::loadFirst( Index& typeno, Index& termno, Index& df)
{
	char prefix = (char)KeyPrefix;
	DatabaseCursorInterface::Slice key( m_cursor->seekFirst( &prefix, 1));
	return getData( key, typeno, termno, df);
}

bool DatabaseAdapter_DocFrequency::Cursor::loadNext( Index& typeno, Index& termno, Index& df)
{
	DatabaseCursorInterface::Slice key( m_cursor->seekNext());
	return getData( key, typeno, termno, df);
}

bool DatabaseAdapter_DocFrequency::load( const DatabaseClientInterface* database, const Index& typeno, const Index& termno, Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	df = 0;
	if (!database->readValue( dbkey.ptr(), dbkey.size(), dfstr, DatabaseOptions().useCache())) return false;

	char const* cc = dfstr.c_str();
	df = unpackIndex( cc, cc + dfstr.size());
	return true;
}

Index DatabaseAdapter_DocFrequency::get( const DatabaseClientInterface* database, const Index& typeno, const Index& termno)
{
	Index rt;
	if (!load( database, typeno, termno, rt)) return 0;
	return rt;
}

void DatabaseAdapter_DocFrequency::store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	packIndex( dfstr, df);
	transaction->write( dbkey.ptr(), dbkey.size(), dfstr.c_str(), dfstr.size());
}

void DatabaseAdapter_DocFrequency::remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	transaction->remove( dbkey.ptr(), dbkey.size());
}

void DatabaseAdapter_DocFrequency::storeImm( DatabaseClientInterface* database, const Index& typeno, const Index& termno, const Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	packIndex( dfstr, df);
	database->writeImm( dbkey.ptr(), dbkey.size(), dfstr.c_str(), dfstr.size());
}


bool DatabaseAdapter_MetaDataDescr::load( const DatabaseClientInterface* database, std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	return database->readValue( dbkey.ptr(), dbkey.size(), descr, DatabaseOptions());
}

void DatabaseAdapter_MetaDataDescr::storeImm( DatabaseClientInterface* database, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	database->writeImm( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}

void DatabaseAdapter_MetaDataDescr::store( DatabaseTransactionInterface* transaction, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	transaction->write( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}


