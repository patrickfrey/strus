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
#include "dataBlockStorage.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "indexPacker.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

DataBlockStorage::DataBlockStorage( DatabaseInterface* database_, const DatabaseKey& key_, bool useLruCache_)
	:m_database(database_)
	,m_cursor(database_->createCursor( useLruCache_))
	,m_key(key_)
	,m_keysize(key_.size())
	,m_curblock(key_.prefix())
{}

const DataBlock* DataBlockStorage::extractData(
		const DatabaseCursorInterface::Slice& key,
		const DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr() + m_keysize;
	const char* ke = ki + key.size() - m_keysize;
	Index id = unpackIndex( ki, ke);

	m_curblock.init( id, value.ptr(), value.size());
	return &m_curblock;
}

const DataBlock* DataBlockStorage::load( const Index& id)
{
	m_key.resize( m_keysize);
	m_key.addElem( id);
	DatabaseCursorInterface::Slice
		key = m_cursor->seekUpperBound( m_key.ptr(), m_key.size(), m_keysize);
	if (!key.defined()) return 0;

	return extractData( key, m_cursor->value());
}

const DataBlock* DataBlockStorage::loadFirst()
{
	DatabaseCursorInterface::Slice
		key = m_cursor->seekFirst( m_key.ptr(), m_keysize);

	return extractData( key, m_cursor->value());
}

const DataBlock* DataBlockStorage::loadNext()
{
	DatabaseCursorInterface::Slice key = m_cursor->seekNext();
	return extractData( key, m_cursor->value());
}

const DataBlock* DataBlockStorage::loadLast()
{
	DatabaseCursorInterface::Slice key = m_cursor->seekLast( m_key.ptr(), m_keysize);
	return extractData( key, m_cursor->value());
}

void DataBlockStorage::store(
	const DataBlock& block,
	DatabaseTransactionInterface* transaction)
{
	m_key.resize( m_keysize);
	if (!block.id())
	{
		throw std::runtime_error("trying to store data block with id NULL");
	}
	m_key.addElem( block.id());
	transaction->write( m_key.ptr(), m_key.size(), block.charptr(), block.size());
}

void DataBlockStorage::dispose(
	const Index& id,
	DatabaseTransactionInterface* transaction)
{
	m_key.resize( m_keysize);
	m_key.addElem( id);

	transaction->remove( m_key.ptr(), m_key.size());
}


