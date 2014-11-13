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
#include "metaDataReader.hpp"
#include "databaseKey.hpp"
#include <leveldb/write_batch.h>
#include <cstring>

using namespace strus;

void MetaDataBlockMap::defineMetaData( Index docno, char varname, float value)
{
	Index blockno = MetaDataBlock::blockno( docno);
	MetaDataKey key( varname, blockno);
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.find( key);
	if (mi == m_map.end())
	{
		MetaDataBlock* blk = MetaDataReader::readBlockFromDB( blockno, varname);
		MetaDataBlockReference& block = m_map[ key];
		block.reset( blk);
		block->setValue( docno, value);
	}
	else
	{
		mi->second->setValue( docno, value);
	}
}

void MetaDataBlockMap::flush()
{
	leveldb::WriteBatch batch;

	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (!mi->second.get()) continue;

		DatabaseKey key( (char)DatabaseKey::DocMetaDataPrefix,
				 mi->first.first, mi->second->blockno());

		leveldb::Slice keyslice( key.ptr(), key.size());
		leveldb::Slice valueslice(
			(const char*)mi->second->data(),
			MetaDataBlock::MetaDataBlockSize*sizeof(float));
		batch.Put( keyslice, valueslice);
	}
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	batch.Clear();
	mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		/// NOTE: It cannot be guaranteed here that metadata is correct for
		/// the moment of update/delete ! But it is quaranted that a 
		/// reload occurs in insert.
		m_cache->resetBlock( mi->second->blockno(), mi->first.first);
	}
	m_map.clear();
}



