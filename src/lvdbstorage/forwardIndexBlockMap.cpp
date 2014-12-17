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
#include "forwardIndexBlockMap.hpp"
#include "keyMap.hpp"

using namespace strus;

void ForwardIndexBlockMap::defineForwardIndexTerm(
	const Index& typeno,
	const Index& docno,
	const Index& pos,
	const std::string& termstring)
{
	BlockKey dbkey( typeno, docno);

	Map::iterator mi = m_map.find( dbkey.index());
	if (mi == m_map.end())
	{
		m_map[ dbkey.index()].define( pos, termstring);
	}
	else
	{
		mi->second.define( pos, termstring);
	}
}

void ForwardIndexBlockMap::deleteForwardIndexTerm(
	const Index& typeno,
	const Index& docno,
	const Index& pos)
{
	defineForwardIndexTerm( typeno, docno, pos, std::string());
}

void ForwardIndexBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		typename ForwardIndexBlockElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		if (ei == ee) continue;
		Index lastInsertBlockId = mi->second.lastInsertBlockId();

		BlockStorage<ForwardIndexBlock> blkstorage(
				m_db, DatabaseKey::ForwardIndexPrefix,
				BlockKey(mi->first), false);
		const ForwardIndexBlock* blk;
		ForwardIndexBlock newblk;

		// [1] Delete all old blocks with the database key as prefix address:
		for (blk = blkstorage.load( 0);
			blk != 0; blk = blkstorage.loadNext())
		{
			blkstorage.dispose( blk->id(), batch);
		}
		// [2] Write the new blocks:
		insertNewElements( blkstorage, ei, ee, newblk, lastInsertBlockId, batch);
	}
	m_map.clear();
}

void ForwardIndexBlockMap::insertNewElements(
		BlockStorage<ForwardIndexBlock>& blkstorage,
		ForwardIndexBlockElementMap::const_iterator& ei,
		const ForwardIndexBlockElementMap::const_iterator& ee,
		ForwardIndexBlock& newblk,
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
