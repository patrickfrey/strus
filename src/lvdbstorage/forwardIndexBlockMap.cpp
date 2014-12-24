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
		m_blockar.push_back( BlockListElem( ForwardIndexBlock(), 0));
		m_blockar.back().first.setId( MaxBlockId);
		m_blockar.back().first.append( pos, termstring);
		m_map[ dbkey.index()] = m_blockar.size()-1;
	}
	else
	{
		BlockListElem& elem = m_blockar[ mi->second];
		if (elem.first.full())
		{
			Index lastpos = elem.first.position_at( elem.first.prevItem( elem.first.charend()));
			elem.first.setId( lastpos);
			std::size_t nextblock = mi->second;
			mi->second = m_blockar.size();

			m_blockar.push_back( BlockListElem( ForwardIndexBlock(), nextblock));
			m_blockar.back().first.setId( MaxBlockId);
			m_blockar.back().first.append( pos, termstring);
		}
		else
		{
			elem.first.append( pos, termstring);
		}
	}
}

void ForwardIndexBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		std::vector<ForwardIndexBlock*> blklist;

		std::size_t blkidx = mi->second;
		do
		{
			BlockListElem& elem = m_blockar[ blkidx];
			blklist.push_back(&elem.first);
			blkidx = elem.second;
		}
		while (blkidx);

		BlockStorage<ForwardIndexBlock> blkstorage(
				m_db, DatabaseKey::ForwardIndexPrefix,
				BlockKey(mi->first), false);
		const ForwardIndexBlock* blk;

		// [1] Delete all old blocks with the database key as prefix address:
		for (blk = blkstorage.load( 0);
			blk != 0; blk = blkstorage.loadNext())
		{
			blkstorage.dispose( blk->id(), batch);
		}

		// [2] Write the new blocks:
		std::vector<ForwardIndexBlock*>::const_iterator bi = blklist.begin(), be = blklist.end();
		for (; bi != be; ++bi)
		{
			if ((*bi)->id() == MaxBlockId)
			{
				Index lastpos = (*bi)->position_at( (*bi)->prevItem( (*bi)->charend()));
				(*bi)->setId( lastpos);
				blkstorage.store( **bi, batch);
			}
		}
	}
	m_map.clear();
}

