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
#include "posinfoBlockMap.hpp"
#include "keyMap.hpp"

using namespace strus;

void PosinfoBlockMap::definePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	const std::vector<Index>& pos)
{
	BlockKey dbkey( termtype, termvalue);

	Map::iterator mi = m_map.find( dbkey.index());
	if (mi == m_map.end())
	{
		m_map[ dbkey.index()].define( docno, pos);
	}
	else
	{
		mi->second.define( docno, pos);
	}
}

void PosinfoBlockMap::deletePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno)
{
	definePosinfoPosting( termtype, termvalue, docno, std::vector<Index>());
}

void PosinfoBlockMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
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
				throw std::runtime_error( "internal: term value undefined (posinfo map)");
			}
			BlockKey newkey( dbkey.elem(1), ri->second);

			PosinfoBlockElementMap& newelem = m_map[ newkey.index()];
			newelem.swap( mi->second);
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void PosinfoBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		PosinfoBlockElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		if (ei == ee) continue;
		Index lastInsertBlockId = mi->second.lastInsertBlockId();

		BlockStorage<PosinfoBlock> blkstorage( m_db, BlockKey(mi->first), false);
		PosinfoBlock newblk;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewElements( blkstorage, ei, ee, newblk, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
	}
	m_map.clear();
}
	
void PosinfoBlockMap::insertNewElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		PosinfoBlockElementMap::const_iterator& ei,
		const PosinfoBlockElementMap::const_iterator& ee,
		PosinfoBlock& newblk,
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
		newblk.append( ei->docno(), ei->ptr());
		blkid = ei->docno();
	}
	if (!newblk.empty())
	{
		newblk.setId( blkid);
		blkstorage.store( newblk, batch);
	}
}

void PosinfoBlockMap::mergeNewElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		PosinfoBlockElementMap::const_iterator& ei,
		const PosinfoBlockElementMap::const_iterator& ee,
		PosinfoBlock& newblk,
		leveldb::WriteBatch& batch)
{
	const PosinfoBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->docno())))
	{
		PosinfoBlockElementMap::const_iterator newblk_start = ei;
		for (; ei != ee && ei->docno() <= blk->id(); ++ei){}

		newblk = PosinfoBlockElementMap::merge( newblk_start, ei, *blk);
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
				blkstorage.dispose( newblk.id(), batch);
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
	

