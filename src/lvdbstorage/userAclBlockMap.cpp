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
#include "userAclBlockMap.hpp"
#include "keyMap.hpp"

using namespace strus;

void UserAclBlockMap::markSetElement(
	const Index& userno,
	const Index& elemno,
	bool isMember)
{
	Map::iterator mi = m_map.find( userno);
	if (mi == m_map.end())
	{
		m_map[ userno][ elemno] = isMember;
	}
	else
	{
		mi->second[ elemno] = isMember;
	}
}

void UserAclBlockMap::definePosting(
	const Index& userno,
	const Index& elemno)
{
	markSetElement( userno, elemno, true);
}

void UserAclBlockMap::deletePosting(
	const Index& userno,
	const Index& elemno)
{
	markSetElement( userno, elemno, false);
}

void UserAclBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		BooleanBlockElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		if (ei == ee) continue;
		Index lastInsertBlockId = mi->second.lastInsertBlockId();

		BlockStorage<BooleanBlock> blkstorage(
				m_db, DatabaseKey::UserAclBlockPrefix,
				BlockKey(mi->first), false);
		BooleanBlock newblk( (char)DatabaseKey::UserAclBlockPrefix);

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewElements( blkstorage, ei, ee, newblk, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
	}
	m_map.clear();
}
	
void UserAclBlockMap::insertNewElements(
		BlockStorage<BooleanBlock>& blkstorage,
		BooleanBlockElementMap::const_iterator& ei,
		const BooleanBlockElementMap::const_iterator& ee,
		BooleanBlock& newblk,
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
		if (ei->second)
		{
			newblk.defineElement( ei->first);
		}
		blkid = ei->first;
	}
	if (!newblk.empty())
	{
		newblk.setId( blkid);
		blkstorage.store( newblk, batch);
	}
}

void UserAclBlockMap::mergeNewElements(
		BlockStorage<BooleanBlock>& blkstorage,
		BooleanBlockElementMap::const_iterator& ei,
		const BooleanBlockElementMap::const_iterator& ee,
		BooleanBlock& newblk,
		leveldb::WriteBatch& batch)
{
	const BooleanBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->first)))
	{
		BooleanBlockElementMap::const_iterator newblk_start = ei;
		for (; ei != ee && ei->first <= blk->id(); ++ei){}

		newblk = BooleanBlockElementMap::merge( newblk_start, ei, *blk);
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
	

