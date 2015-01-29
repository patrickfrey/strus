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
#include "databaseRecord.hpp"
#include "indexPacker.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseCursorInterface.hpp"

using namespace strus;

Index DatabaseRecord_StringIndex_Base::get( char prefix, const DatabaseInterface* database, const std::string& key)
{
	Index rt;
	if (!DatabaseRecord_StringIndex_Base::load( prefix, database, key, rt)) return 0;
	return rt;
}

bool DatabaseRecord_StringIndex_Base::load( char prefix, const DatabaseInterface* database, const std::string& key, Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key.c_str(), key.size());
	std::string valuestr;
	value = 0;
	if (!database->readValue( keystr.c_str(), keystr.size(), valuestr, false)) return false;

	char const* cc = valuestr.c_str();
	value = unpackIndex( cc, cc + valuestr.size());
	return true;
}

void DatabaseRecord_StringIndex_Base::store( char prefix, DatabaseTransactionInterface* transaction, const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key.c_str(), key.size());
	std::string valuestr;
	packIndex( valuestr, value);
	transaction->write( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}

void DatabaseRecord_StringIndex_Base::remove( char prefix, DatabaseTransactionInterface* transaction, const std::string& key)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key.c_str(), key.size());
	transaction->remove( keystr.c_str(), keystr.size());
}

void DatabaseRecord_StringIndex_Base::storeImm( char prefix, DatabaseInterface* database, const std::string& key, const Index& value)
{
	std::string keystr;
	keystr.push_back( prefix);
	keystr.append( key.c_str(), key.size());
	std::string valuestr;
	packIndex( valuestr, value);
	database->writeImm( keystr.c_str(), keystr.size(), valuestr.c_str(), valuestr.size());
}


bool DatabaseRecord_DocAttribute::load( const DatabaseInterface* database, const Index& docno, const Index& attrno, std::string& value)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return database->readValue( dbkey.ptr(), dbkey.size(), value, true);
}

void DatabaseRecord_DocAttribute::store( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno, const char* value)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return transaction->write( dbkey.ptr(), dbkey.size(), value, std::strlen(value));
}

void DatabaseRecord_DocAttribute::remove( DatabaseTransactionInterface* transaction, const Index& docno, const Index& attrno)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( docno, attrno));
	return transaction->remove( dbkey.ptr(), dbkey.size());
}

void DatabaseRecord_DocAttribute::removeAll( DatabaseTransactionInterface* transaction, const Index& docno)
{
	DatabaseKey dbkey( KeyPrefix, docno);
	return transaction->removeSubTree( dbkey.ptr(), dbkey.size());
}


bool DatabaseRecord_DocFrequency::load( const DatabaseInterface* database, const Index& typeno, const Index& termno, Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	df = 0;
	if (!database->readValue( dbkey.ptr(), dbkey.size(), dfstr, false)) return false;

	char const* cc = dfstr.c_str();
	df = unpackIndex( cc, cc + dfstr.size());
	return true;
}

Index DatabaseRecord_DocFrequency::get( const DatabaseInterface* database, const Index& typeno, const Index& termno)
{
	Index rt;
	if (!load( database, typeno, termno, rt)) return 0;
	return rt;
}

void DatabaseRecord_DocFrequency::store( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno, const Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	packIndex( dfstr, df);
	transaction->write( dbkey.ptr(), dbkey.size(), dfstr.c_str(), dfstr.size());
}

void DatabaseRecord_DocFrequency::remove( DatabaseTransactionInterface* transaction, const Index& typeno, const Index& termno)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	transaction->remove( dbkey.ptr(), dbkey.size());
}

void DatabaseRecord_DocFrequency::storeImm( DatabaseInterface* database, const Index& typeno, const Index& termno, const Index& df)
{
	DatabaseKey dbkey( KeyPrefix, BlockKey( typeno, termno));
	std::string dfstr;
	packIndex( dfstr, df);
	database->writeImm( dbkey.ptr(), dbkey.size(), dfstr.c_str(), dfstr.size());
}


bool DatabaseRecord_MetaDataDescr::load( const DatabaseInterface* database, std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	return database->readValue( dbkey.ptr(), dbkey.size(), descr, false);
}

void DatabaseRecord_MetaDataDescr::storeImm( DatabaseInterface* database, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	database->writeImm( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}

void DatabaseRecord_MetaDataDescr::store( DatabaseTransactionInterface* transaction, const std::string& descr)
{
	DatabaseKey dbkey( KeyPrefix);
	transaction->write( dbkey.ptr(), dbkey.size(), descr.c_str(), descr.size());
}


bool DatabaseRecord_DocMetaData::load( const DatabaseInterface* database, const Index& blockno, std::string& blk)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	return database->readValue( dbkey.ptr(), dbkey.size(), blk, false);
}

void DatabaseRecord_DocMetaData::store( DatabaseTransactionInterface* transaction, const Index& blockno, const char* blk, std::size_t blksize)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	transaction->write( dbkey.ptr(), dbkey.size(), blk, blksize);
}

void DatabaseRecord_DocMetaData::remove( DatabaseTransactionInterface* transaction, const Index& blockno)
{
	DatabaseKey dbkey( KeyPrefix, blockno);
	transaction->remove( dbkey.ptr(), dbkey.size());
}


