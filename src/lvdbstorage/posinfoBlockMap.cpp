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
#ifndef _STRUS_LVDB_POSINFO_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_POSINFO_BLOCK_MAP_HPP_INCLUDED
#include "posinfoBlockMap.hpp"

using namespace strus;

void PosinfoBlockMap::definePosinfo(
		const Index& docno,
		const std::vector<Index>& pos)
{
	boost::mutex::scoped_lock( m_mutex);
	m_map[ docno] = pos;
}

void PosinfoBlockMap::deletePosinfo(
		const Index& docno)
{
	boost::mutex::scoped_lock( m_mutex);
	m_map[ docno].clear();
}

void PosinfoBlockMap::writeBlock(
	leveldb::WriteBatch& batch,
	const Index& docno,
	const PosinfoBlock& blk)
{
	
}

void PosinfoBlockMap::deleteBlock(
	leveldb::WriteBatch& batch,
	const Index& docno)
{
	
}

void PosinfoBlockMap::writeMergeBlock(
	leveldb::WriteBatch& batch,
	const Index& typeno,
	const Index& valueno,
	Map::const_iterator& ei,
	const Map::const_iterator& ee,
	const PosinfoBlock& blk)
{
	
}

void PosinfoBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		std::vector<Index>::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		DocnoBlockReader blkreader( m_db, mi->first.type, mi->first.value);
		const DocnoBlock* blk;

		while (!!(blk=blkreader.readBlock( ei->first)))
		{
			writeMergeBlock(
				batch, mi->first.type, mi->first.value,
				ei, ee, blk);	
		}
		std::size_t newblksize = 0;
		DocnoBlock::Element newblk[ BlockSize];

		// Join new elements with last block to a block with size BlockSize:
		blk = blkreader.readLastBlock();
		if (blk)
		{
			if (blk->size() < BlockSize)
			{
				deleteBlock(
					batch, mi->first.type,
					mi->first.value, blk->back().docno());
				while (newblksize < blk->size())
				{
					newblk[ newblksize] = blk->data()[ newblksize];
					++newblksize;
				}
			}
		}
		// Write rest elements to new blocks:
		while (ei != ee)
		{
			for (;newblksize < BlockSize && ei != ee; ++ei,++newblksize)
			{
				newblk[ newblksize] = ei->second;
			}
			writeBlock( batch, mi->first.type, mi->first.value,
					newblk, newblksize);
			newblksize = 0;
		}
		if (newblksize)
		{
			writeBlock( batch, mi->first.type, mi->first.value,
					newblk, newblksize);
		}
	}
	m_map.clear();
}




