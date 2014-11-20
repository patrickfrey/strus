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
#include "metaDataReader.hpp"
#include "databaseKey.hpp"
#include <leveldb/write_batch.h>
#include <cstring>

using namespace strus;

MetaDataBlockMap::~MetaDataBlockMap()
{
	if (m_itr)
	{
		delete m_itr;
	}
}

void MetaDataBlockMap::deleteMetaData( Index docno)
{
	if (!m_itr)
	{
		m_itr = m_db->NewIterator( leveldb::ReadOptions());
	}
	unsigned int blockno = MetaDataBlock::blockno( docno);
	DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix, blockno);

	m_itr->Seek( leveldb::Slice( key.ptr(), key.size()));
	if (!m_itr->Valid()) return;

	while (m_itr->Valid()
	&&  key.size() <= m_itr->key().size()
	&&  0==std::memcmp( key.ptr(), m_itr->key().data(), key.size()))
	{
		char const* ki = m_itr->key().data() + key.size();
		const char* ke = ki + m_itr->key().size() - key.size();
		if (ki+1 != ke) throw std::runtime_error( "corrupt data in storage (illegal meta data key)");
		char varname = *ki;

		defineMetaData( docno, varname, 0.0);
		m_itr->Next();
	}
}

void MetaDataBlockMap::defineMetaData( Index docno, char varname, float value)
{
	unsigned int blockno = MetaDataBlock::blockno( docno);
	MetaDataKey key( varname, blockno);
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.find( key);
	if (mi == m_map.end())
	{
		MetaDataBlock* blk = MetaDataReader::readBlockFromDB( m_db, blockno, varname);
		MetaDataBlockReference& block = m_map[ key];
		block.reset( blk);
		block->setValue( docno, value);
	}
	else
	{
		mi->second->setValue( docno, value);
	}
}

void MetaDataBlockMap::getWriteBatch(
	leveldb::WriteBatch& batch,
	MetaDataBlockCache& cache)
{
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (!mi->second.get()) continue;
		cache.declareVoid( mi->second->blockno(), mi->first.first);

		DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix,
				 mi->second->blockno(), mi->first.first);

		leveldb::Slice keyslice( key.ptr(), key.size());
		leveldb::Slice valueslice(
			(const char*)mi->second->data(),
			MetaDataBlock::MetaDataBlockSize*sizeof(float));
		batch.Put( keyslice, valueslice);
	}
	m_map.clear();
}


