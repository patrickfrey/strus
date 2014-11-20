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
#ifndef _STRUS_LVDB_DATA_BLOCK_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_DATA_BLOCK_STORAGE_HPP_INCLUDED
#include "dataBlock.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <cstdlib>

namespace strus {

/// \class DataBlockStorage
class DataBlockStorage
{
public:
	DataBlockStorage( leveldb::DB* db_, const DatabaseKey& key_, bool useLruCache_)
		:m_db(db_)
		,m_itr(0)
		,m_key(key_)
		,m_keysize(key_.size())
		,m_curblock(key_.prefix())
	{
		m_readOptions.fill_cache = useLruCache_;
	}
	DataBlockStorage( const DataBlockStorage& o)
		:m_db(o.m_db)
		,m_itr(0)
		,m_readOptions(o.m_readOptions)
		,m_key(o.m_key)
		,m_keysize(o.m_keysize)
		,m_curblock(o.m_curblock){}
	
	virtual ~DataBlockStorage()
	{
		if (m_itr)
		{
			delete m_itr;
		}
	}

	const DataBlock* curblock() const
	{
		return &m_curblock;
	}

	const DataBlock* load( const Index& id);
	const DataBlock* loadLast();
	const DataBlock* loadNext();

	void store( const DataBlock& block, leveldb::WriteBatch& batch);
	void dispose( const Index& id, leveldb::WriteBatch& batch);

private:
	const DataBlock* extractData();

private:
	leveldb::DB* m_db;
	leveldb::Iterator* m_itr;
	leveldb::ReadOptions m_readOptions;
	DatabaseKey m_key;
	std::size_t m_keysize;
	DataBlock m_curblock;
};

} //namespace
#endif

