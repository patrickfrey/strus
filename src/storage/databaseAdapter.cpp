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
#include "posinfoBlock.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"

using namespace strus;

Index DatabaseAdapter_StringIndex_Base::get( char prefix, const DatabaseInterface* database, const std::string& key)
{
	Index rt;
	if (!DatabaseAdapter_StringIndex_Base::load( prefix, database, key, rt)) return 0;
	return rt;
}

bool DatabaseAdapter_StringIndex_Base::load( char prefix, const DatabaseInterface* database, const std::string& key, Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key);
	std::string valuestr;
	value = 0;
	if (!database->readValue( keystr.c_str(), keystr.size(), valuestr, false)) return false;

	char const* cc = valuestr.c_str();
	value = unpackIndex( cc, cc + valuestr.size());
	return true;
}

void DatabaseAdapter_StringIndex_Base::store( char prefix, DatabaseTransactionInterface* transaction, const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key);
	std::string valuestr;
	packIndex( valuestr, value);
	transaction->write( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}

void DatabaseAdapter_StringIndex_Base::remove( char prefix, DatabaseTransactionInterface* transaction, const std::string& key)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key);
	transaction->remove( keystr.c_str(), keystr.size());
}

void DatabaseAdapter_StringIndex_Base::storeImm( char prefix, DatabaseInterface* database, const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key);
	std::string valuestr;
	packIndex( valuestr, value);
	database->writeImm( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}


bool DatabaseAdapter_PosinfoBlock::loadUpperBound( DatabaseCursorInterface* cursor, const Index& typeno, const Index& termno, const Index& docno, PosinfoBlock& blk)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey(typeno,termno));
	std::size_t domainkeysize = dbkey.size();
	dbkey.addElem( docno);
	DatabaseCursorInterface::Slice
		key = cursor->seekUpperBound( dbkey.ptr(), dbkey.size(), domainkeysize);
	if (!key.defined()) return false;
	char const* ki = key.ptr()+domainkeysize;
	char const* ke = ki + key.size()-domainkeysize;
	Index blkdocno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = cursor->value();
	blk.init( blkdocno, blkslice.ptr(), blkslice.size());
	return true;
}

bool DatabaseAdapter_PosinfoBlock::loadFirst( DatabaseCursorInterface* cursor, const Index& typeno, const Index& termno, PosinfoBlock& blk)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey(typeno,termno));
	DatabaseCursorInterface::Slice
		key = cursor->seekFirst( dbkey.ptr(), dbkey.size());
	if (!key.defined()) return false;
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	Index docno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = cursor->value();
	blk.init( docno, blkslice.ptr(), blkslice.size());
	return true;
}

bool DatabaseAdapter_PosinfoBlock::loadNext( DatabaseCursorInterface* cursor, PosinfoBlock& blk)
{
	DatabaseCursorInterface::Slice key = cursor->seekNext();
	if (!key.defined()) return false;
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	Index docno = unpackIndex( ki, ke);
	DatabaseCursorInterface::Slice blkslice = cursor->value();
	blk.init( docno, blkslice.ptr(), blkslice.size());
	return true;
}

void DatabaseAdapter_PosinfoBlock::store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const PosinfoBlock& blk)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey(typeno,termno), blk.id());
	transaction->write( dbkey.ptr(), dbkey.size(), blk.charptr(), blk.size());
}

void DatabaseAdapter_PosinfoBlock::remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& docno)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey(typeno,termno), docno);
	transaction->remove( dbkey.ptr(), dbkey.size());
}

bool DatabaseAdapter_DocAttribute::load( const DatabaseInterface* database, const Index& docno, const Index& attrno, std::string& value)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return database->readValue( dbkey.ptr(), dbkey.size(), value, true);
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


bool DatabaseAdapter_DocFrequency::load( const DatabaseInterface* database, const Index& typeno, const Index& termno, Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	df = 0;
	if (!database->readValue( dbkey.ptr(), dbkey.size(), dfstr, false)) return false;

	char const* cc = dfstr.c_str();
	df = unpackIndex( cc, cc + dfstr.size());
	return true;
}

Index DatabaseAdapter_DocFrequency::get( const DatabaseInterface* database, const Index& typeno, const Index& termno)
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

void DatabaseAdapter_DocFrequency::storeImm( DatabaseInterface* database, const Index& typeno, const Index& termno, const Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	packIndex( dfstr, df);
	database->writeImm( dbkey.ptr(), dbkey.size(), dfstr.c_str(), dfstr.size());
}


bool DatabaseAdapter_MetaDataDescr::load( const DatabaseInterface* database, std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	return database->readValue( dbkey.ptr(), dbkey.size(), descr, false);
}

void DatabaseAdapter_MetaDataDescr::storeImm( DatabaseInterface* database, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	database->writeImm( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}

void DatabaseAdapter_MetaDataDescr::store( DatabaseTransactionInterface* transaction, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	transaction->write( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}


MetaDataBlock* DatabaseAdapter_DocMetaData::load( const DatabaseInterface* database, const MetaDataDescription* descr, const Index& blockno)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	std::string blkstr;
	if (!database->readValue( dbkey.ptr(), dbkey.size(), blkstr, false)) return 0;
	return new MetaDataBlock( descr, blockno, blkstr.c_str(), blkstr.size());
}

bool DatabaseAdapter_DocMetaData::seek( DatabaseCursorInterface* cursor, const Index& blockno)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	return cursor->seekUpperBound( dbkey.ptr(), dbkey.size(), dbkey.size()).defined();
}

MetaDataBlock* DatabaseAdapter_DocMetaData::loadNext( DatabaseCursorInterface* cursor, const MetaDataDescription* descr)
{
	DatabaseCursorInterface::Slice key = cursor->seekNext();
	if (!key.defined()) return 0;
	DatabaseCursorInterface::Slice blk = cursor->value();
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	Index blockno = unpackIndex( ki, ke);
	return new MetaDataBlock( descr, blockno, blk.ptr(), blk.size());
}

MetaDataBlock* DatabaseAdapter_DocMetaData::loadFirst( DatabaseCursorInterface* cursor, const MetaDataDescription* descr)
{
	DatabaseKey dbkey( KeyPrefix);
	DatabaseCursorInterface::Slice key = cursor->seekFirst( dbkey.ptr(), dbkey.size());
	if (!key.defined()) return 0;
	DatabaseCursorInterface::Slice blk = cursor->value();
	char const* ki = key.ptr()+1;
	char const* ke = ki + key.size()-1;
	Index blockno = unpackIndex( ki, ke);
	return new MetaDataBlock( descr, blockno, blk.ptr(), blk.size());
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


