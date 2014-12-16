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
#include "docnoBlockMap.hpp"

using namespace strus;

void DocnoBlockMap::defineDocnoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	unsigned int ff,
	float weight)
{
	BlockKey dbkey( termtype, termvalue);
	DocnoBlockElement elem( docno, ff, weight);

	Map::iterator mi = m_map.find( dbkey.index());
	if (mi == m_map.end())
	{
		m_map[ dbkey.index()].define( docno, elem);
	}
	else
	{
		mi->second.define( docno, elem);
	}
}

void DocnoBlockMap::deleteDocnoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno)
{
	defineDocnoPosting( termtype, termvalue, docno, 0, 0.0);
}

void DocnoBlockMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
{
	typename Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		BlockKey dbkey( mi->first);

		if (KeyMap::isUnknown( dbkey.elem(2)))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( dbkey.elem(2));
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (docno map)");
			}
			BlockKey newkey( dbkey.elem(1), ri->second);

			DocnoBlockElementMap& newelem = m_map[ newkey.index()];
			newelem.swap( mi->second);
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void DocnoBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		DocnoBlockElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		if (ei == ee) continue;
		Index lastInsertBlockId = mi->second.lastInsertBlockId();

		BlockStorage<DocnoBlock> blkstorage( m_db, BlockKey(mi->first), false);
		DocnoBlock newblk;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewElements( blkstorage, ei, ee, newblk, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
	}
	m_map.clear();
}
	
void DocnoBlockMap::insertNewElements(
		BlockStorage<DocnoBlock>& blkstorage,
		DocnoBlockElementMap::const_iterator& ei,
		const DocnoBlockElementMap::const_iterator& ee,
		DocnoBlock& newblk,
		const Index& lastInsertBlockId,
		leveldb::WriteBatch& batch)
{
	if (newblk.id() < lastInsertBlockId)
	{
		newblk.setId( lastInsertBlockId);
	}
	Index blkid = newblk.id();
	for (; ei != ee; ++ei)
	{
		if (newblk.full())
		{
			newblk.setId( blkid);
			blkstorage.store( newblk, batch);
			newblk.clear();
			newblk.setId( lastInsertBlockId);
		}
		newblk.append( ei->key(), ei->value());
		blkid = ei->key();
	}
	if (!newblk.empty())
	{
		newblk.setId( blkid);
		blkstorage.store( newblk, batch);
	}
}

void DocnoBlockMap::mergeNewElements(
		BlockStorage<DocnoBlock>& blkstorage,
		DocnoBlockElementMap::const_iterator& ei,
		const typename DocnoBlockElementMap::const_iterator& ee,
		DocnoBlock& newblk,
		leveldb::WriteBatch& batch)
{
	const DocnoBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->key())))
	{
		DocnoBlock elemblk;
		elemblk.setId( blk->id());

		for (; ei != ee && ei->key() <= blk->id(); ++ei)
		{
			elemblk.append( ei->key(), ei->value());
		}
		newblk = DocnoBlock::merge( elemblk, *blk);
		if (blkstorage.loadNext())
		{
			// ... is not the last block, so we store it
			blkstorage.store( newblk, batch);
			newblk.clear();
		}
		else
		{
			if (newblk.full())
			{
				// ... it is the last block, but full
				blkstorage.store( newblk, batch);
				newblk.clear();
			}
			else
			{
				blkstorage.dispose( elemblk.id(), batch);
			}
			break;
		}
	}
	if (newblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee &&  0!=(blk=blkstorage.loadLast()))
		{
			newblk.initcopy( *blk);
			blkstorage.dispose( blk->id(), batch);
		}
	}
}
	

