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
#ifndef _STRUS_LVDB_BLOCK_MAP_TEMPLATE_HPP_INCLUDED
#define _STRUS_LVDB_BLOCK_MAP_TEMPLATE_HPP_INCLUDED
#include "strus/index.hpp"
#include "dataBlock.hpp"
#include "blockStorage.hpp"
#include "databaseKey.hpp"
#include <cstdlib>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

template <class BlockType, class BlockElement>
class BlockMap
{
public:
	BlockMap( leveldb::DB* db_)
		:m_db(db_){}
	BlockMap( const BlockMap& o)
		:m_db(o.m_db),m_map(o.m_map){}

	void defineElement(
		const DatabaseKey& dbkey,
		const Index& elemid,
		const BlockElement& elem)
	{
		boost::mutex::scoped_lock( m_mutex);
		typename Map::iterator mi = m_map.find( dbkey);
		if (mi == m_map.end())
		{
			ElementMap em;
			em[ elemid] = elem;
			m_map[ dbkey] = em;
		}
		else
		{
			mi->second[ elemid] = elem;
		}
	}

	void deleteElement(
		const DatabaseKey& dbkey,
		const Index& elemid)
	{
		defineElement( dbkey, elemid, BlockElement());
	}

	void getWriteBatch( leveldb::WriteBatch& batch)
	{
		boost::mutex::scoped_lock( m_mutex);
		typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
		for (; mi != me; ++mi)
		{
			typename ElementMap::const_iterator
				ei = mi->second.begin(),
				ee = mi->second.end();
	
			BlockStorage<BlockType> blkstorage( m_db, mi->first, false);
			const BlockType* blk;
			BlockType newblk;
	
			// [1] Merge new elements with existing upper bound blocks:
			while (0!=(blk=blkstorage.load( ei->first)))
			{
				newblk.clear();
				Index blkid = blk->id();
				newblk.setId( blkid);
				for (; ei != ee && ei->first <= blkid; ++ei)
				{
					newblk.append( ei->first, ei->second);
				}
				if (0 != blkstorage.loadNext())
				{
					// ... is not the last block, so we store it
					blkstorage.store( BlockType::merge( newblk, *blk), batch);
					newblk.clear();
				}
				else
				{
					blkstorage.dispose( blkid, batch);
				}
			}
			if (!newblk.empty())
			{
				// [2.1] 'newblk' is already the last block merged with new elements
			}
			else
			{
				// [2.2] Fill first new block with elements of last 
				// block and dispose the last block:
				if (ei != ee &&  0!=(blk=blkstorage.loadLast()))
				{
					newblk.clear();
					newblk = *blk;
					blkstorage.dispose( blk->id(), batch);
				}
			}
			// [3] Write the new blocks:
			Index blkid = (ei==ee)?newblk.id():ei->first;
			if (ei != ee)
			{
				newblk.setId( mi->second.rbegin()->first);
			}
			for (; ei != ee; ++ei)
			{
				if (newblk.full())
				{
					newblk.setId( blkid);
					blkstorage.store( newblk, batch);
					newblk.clear();
					newblk.setId( mi->second.rbegin()->first);
				}
				newblk.append( ei->first, ei->second);
				blkid = ei->first;
			}
			// [4] Write the new last (incomplete) block, if not empty:
			if (!newblk.empty())
			{
				newblk.setId( blkid);
				blkstorage.store( newblk, batch);
			}
		}
		m_map.clear();
	}

private:
	typedef std::map<Index,BlockElement> ElementMap;
	typedef std::map<DatabaseKey,ElementMap> Map;

private:
	leveldb::DB* m_db;
	boost::mutex m_mutex;
	Map m_map;
};

}
#endif

