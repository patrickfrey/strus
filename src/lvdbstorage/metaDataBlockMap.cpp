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
#include "metaDataBlockMap.hpp"
#include "metaDataBlockCache.hpp"
#include "keyValueStorage.hpp"
#include "databaseKey.hpp"
#include <leveldb/write_batch.h>
#include <cstring>

using namespace strus;

MetaDataBlockMap::~MetaDataBlockMap()
{
}

void MetaDataBlockMap::deleteMetaData( Index docno)
{
	getRecord( docno).clear();
}

void MetaDataBlockMap::deleteMetaData( Index docno, const std::string& varname)
{
	getRecord( docno).clearValue( m_descr.get( m_descr.getHandle( varname)));
}

MetaDataRecord MetaDataBlockMap::getRecord( Index docno)
{
	unsigned int blockno = MetaDataBlock::blockno( docno);
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.find( blockno);
	if (mi == m_map.end())
	{
		KeyValueStorage storage( m_db, DatabaseKey::DocMetaDataPrefix, false);
		const KeyValueStorage::Value* mv = storage.load( KeyValueStorage::Key( blockno));
		MetaDataBlockReference& block = m_map[ blockno];
		block.reset( new MetaDataBlock( &m_descr, blockno, mv->ptr(), mv->size()));
		return (*block)[ MetaDataBlock::index( docno)];
	}
	else
	{
		return (*mi->second)[ MetaDataBlock::index( docno)];
	}
}

void MetaDataBlockMap::defineMetaData( Index docno, const std::string& varname, const Variant& value)
{
	switch (value.type)
	{
		case Variant::Null:
			deleteMetaData( docno, varname);
			break;
		case Variant::Int:
			getRecord( docno).setValueInt( m_descr.get( m_descr.getHandle( varname)), value);
			break;
		case Variant::UInt:
			getRecord( docno).setValueUInt( m_descr.get( m_descr.getHandle( varname)), value);
			break;
		case Variant::Float:
			getRecord( docno).setValueFloat( m_descr.get( m_descr.getHandle( varname)), value);
			break;
	}
}

void MetaDataBlockMap::getWriteBatch(
	leveldb::WriteBatch& batch,
	MetaDataBlockCache& cache)
{
	KeyValueStorage storage( m_db, DatabaseKey::DocMetaDataPrefix, false);

	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (!mi->second.get()) continue;
		cache.declareVoid( mi->second->blockno());

		storage.store( KeyValueStorage::Key( mi->second->blockno()),
				KeyValueStorage::Value( mi->second->charptr(), mi->second->bytesize()),
				batch);
	}
	m_map.clear();
}


