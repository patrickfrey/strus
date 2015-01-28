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
#include "keyValueStorage.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/reference.hpp"
#include <boost/scoped_ptr.hpp>
#include <cstring>

using namespace strus;

const KeyValueStorage::Value* KeyValueStorage::load( const std::string& key)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	DatabaseCursorInterface::Slice value = m_dbcursor->readValue( keystr.c_str(), keystr.size());
	if (!value.defined())
	{
		return 0;
	}
	m_curvalue.init( value.ptr(), value.size());
	return &m_curvalue;
}

const KeyValueStorage::Value* KeyValueStorage::load( const BlockKey& key, const Index& subnode)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	DatabaseCursorInterface::Slice value = m_dbcursor->readValue( dbkey.ptr(), dbkey.size());
	if (!value.defined())
	{
		return 0;
	}
	m_curvalue.init( value.ptr(), value.size());
	return &m_curvalue;
}

void KeyValueStorage::store( const std::string& key, const Value& value, DatabaseTransactionInterface* transaction)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	transaction->write( keystr.c_str(), keystr.size(), value.ptr(), value.size());
}

void KeyValueStorage::store( const BlockKey& key, const Index& subnode, const Value& value, DatabaseTransactionInterface* transaction)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	transaction->write( dbkey.ptr(), dbkey.size(), value.ptr(), value.size());
}

void KeyValueStorage::store( const BlockKey& key, const Value& value, DatabaseTransactionInterface* transaction)
{
	DatabaseKey dbkey( m_keyprefix, key);
	transaction->write( dbkey.ptr(), dbkey.size(), value.ptr(), value.size());
}

void KeyValueStorage::storeIm( DatabaseInterface* database, const std::string& key, const Value& value)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	database->writeImm( keystr.c_str(), keystr.size(), value.ptr(), value.size());
}

void KeyValueStorage::storeIm( DatabaseInterface* database, const BlockKey& key, const Index& subnode, const Value& value)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	database->writeImm( dbkey.ptr(), dbkey.size(), value.ptr(), value.size());
}

void KeyValueStorage::storeIm( DatabaseInterface* database, const BlockKey& key, const Value& value)
{
	DatabaseKey dbkey( m_keyprefix, key);
	database->writeImm( dbkey.ptr(), dbkey.size(), value.ptr(), value.size());
}

void KeyValueStorage::dispose( const std::string& key, DatabaseTransactionInterface* transaction)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	transaction->remove( keystr.c_str(), keystr.size());
}

void KeyValueStorage::dispose( const BlockKey& key, const Index& subnode, DatabaseTransactionInterface* transaction)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	transaction->remove( dbkey.ptr(), dbkey.size());
}

void KeyValueStorage::dispose( const BlockKey& key, DatabaseTransactionInterface* transaction)
{
	DatabaseKey dbkey( m_keyprefix, key);
	transaction->remove( dbkey.ptr(), dbkey.size());
}

void KeyValueStorage::disposeSubnodes( const BlockKey& key, DatabaseTransactionInterface* transaction)
{
	DatabaseKey dbkey( m_keyprefix, key);
	Reference<DatabaseCursorInterface> cursor( transaction->createCursor(false));

	DatabaseCursorInterface::Slice keyitr = cursor->seekFirst( dbkey.ptr(), dbkey.size());
	for (; keyitr.defined(); keyitr=cursor->seekNext())
	{
		transaction->remove( keyitr.ptr(), keyitr.size());
	}
}

std::map<std::string,std::string> KeyValueStorage::getMap()
{
	std::map<std::string,std::string> rt;

	DatabaseCursorInterface::Slice key = m_dbcursor->seekFirst( (const char*)&m_keyprefix, 1);
	for (; key.defined(); key=m_dbcursor->seekNext())
	{
		rt[ key] = m_dbcursor->value();
	}
	return rt;
}

std::map<std::string,std::string> KeyValueStorage::getInvMap()
{
	std::map<std::string,std::string> rt;
	DatabaseCursorInterface::Slice key = m_dbcursor->seekFirst( (const char*)&m_keyprefix, 1);
	for (; key.defined(); key=m_dbcursor->seekNext())
	{
		rt[ m_dbcursor->value()] = key;
	}
	return rt;
}


