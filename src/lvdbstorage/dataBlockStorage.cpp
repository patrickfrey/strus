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
#include "indexPacker.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

const DataBlock* DataBlockStorage::extractData()
{
	if (m_itr->Valid()
	&&  m_keysize <= m_itr->key().size()
	&&  0==std::memcmp( m_key.ptr(), m_itr->key().data(), m_keysize))
	{
		char const* ki = m_itr->key().data() + m_keysize;
		const char* ke = ki + m_itr->key().size() - m_keysize;
		Index id = unpackIndex( ki, ke);

		m_curblock.init( id, m_itr->value().data(), m_itr->value().size());
		return &m_curblock;
	}
	else
	{
		return 0;
	}
}

const DataBlock* DataBlockStorage::load( const Index& id)
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( m_readOptions);
	}
	m_key.resize( m_keysize);
	m_key.addElem( id);
	m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
	if (!m_itr->Valid()) return 0;

	return extractData();
}

const DataBlock* DataBlockStorage::loadNext()
{
	if (!m_itr)
	{
		throw std::logic_error("called DataBlockStorage::loadNext without iterator defined");
	}
	m_itr->Next();
	if (!m_itr->Valid()) return 0;

	return extractData();
}

const DataBlock* DataBlockStorage::loadLast()
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( m_readOptions);
	}
	m_key.resize( m_keysize);
	m_key.addPrefix( (char)0xff);
	m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));
	if (!m_itr->Valid())
	{
		m_itr->SeekToLast();
		if (!m_itr->Valid()) return false;
		m_itr->Prev();
	}
	if (!m_itr->Valid()) return false;

	return extractData();
}

void DataBlockStorage::store(
	const DataBlock& block,
	leveldb::WriteBatch& batch)
{
	m_key.resize( m_keysize);
	m_key.addElem( block.id());

	leveldb::Slice keyslice( m_key.ptr(), m_key.size());
	leveldb::Slice valueslice( block.charptr(), block.size());

	batch.Put( keyslice, valueslice);
}

void DataBlockStorage::dispose(
	const Index& id,
	leveldb::WriteBatch& batch)
{
	m_key.resize( m_keysize);
	m_key.addElem( id);

	leveldb::Slice keyslice( m_key.ptr(), m_key.size());

	batch.Delete( keyslice);
}


