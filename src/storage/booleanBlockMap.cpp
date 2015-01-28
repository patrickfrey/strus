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
#include "booleanBlockMap.hpp"

using namespace strus;

void BooleanBlockMap::insertNewElements(
		BlockStorage<BooleanBlock>& blkstorage,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
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
		// Define posinfo block elements (PosinfoBlock):
		if (newblk.full())
		{
			newblk.setId( blkid);
			blkstorage.store( newblk, batch);
			newblk.clear();
			newblk.setId( lastInsertBlockId);
		}
		if (ei->isMember)
		{
			newblk.defineRange( ei->from, ei->to - ei->from);
		}
		blkid = ei->to;
	}
	if (!newblk.empty())
	{
		newblk.setId( blkid);
		blkstorage.store( newblk, batch);
	}
}

void BooleanBlockMap::mergeNewElements(
		BlockStorage<BooleanBlock>& blkstorage,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newblk,
		leveldb::WriteBatch& batch)
{
	const BooleanBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->from)))
	{
		// Merge posinfo block elements (PosinfoBlock):
		Index splitStart = 0;
		Index splitEnd = 0;

		std::vector<BooleanBlock::MergeRange>::iterator newblk_start = ei;
		for (; ei != ee && ei->from <= blk->id(); ++ei)
		{
			if (ei->to > blk->id())
			{
				// ... last element is overlapping block borders, so we split it
				splitStart = blk->id()+1;
				splitEnd = ei->to; 
				ei->to = blk->id();
			}
		}

		newblk = BooleanBlock::merge( newblk_start, ei, *blk);
		if (splitStart)
		{
			// ... last element is overlapping block borders, no we assign the second half of it to the block
			--ei;
			ei->from = splitStart;
			ei->to = splitEnd;
		}
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

