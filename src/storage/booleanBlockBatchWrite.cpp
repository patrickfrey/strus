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
#include "booleanBlockBatchWrite.hpp"
#include "databaseAdapter.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"

using namespace strus;

void BooleanBlockBatchWrite::insertNewElements(
		DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newblk,
		const Index& lastInsertBlockId,
		DatabaseTransactionInterface* transaction)
{
	if (ei == ee)
	{
		if (!newblk.empty())
		{
			dbadapter->store( transaction, newblk);
			newblk.clear();
		}
	}
	while (ei != ee)
	{
		Index blockid = lastInsertBlockId;
		std::vector<BooleanBlock::MergeRange>::iterator bi = ei;
		for (std::size_t mm = newblk.size(); ei != ee && mm < BooleanBlock::MaxBlockSize; ++ei)
		{
			// ... estimate block size approximately
			if (ei->isMember)
			{
				if (ei->from == ei->to)
				{
					mm += 2;
				}
				else
				{
					mm += 4;
				}
			}
			blockid = ei->to;
		}
		// set new block id:
		newblk.setId( blockid);

		// insert member elements:
		for (; bi != ei; ++bi)
		{
			if (bi->isMember)
			{
				newblk.defineRange( bi->from, bi->to - bi->from);
			}
		}
		// store it:
		dbadapter->store( transaction, newblk);
		newblk.clear();
	}
}

void BooleanBlockBatchWrite::mergeNewElements(
		DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newblk,
		DatabaseTransactionInterface* transaction)
{
	BooleanBlock blk;
	while (ei != ee && dbadapter->loadUpperBound( ei->from, blk))
	{
		Index splitStart = 0;
		Index splitEnd = 0;

		std::vector<BooleanBlock::MergeRange>::iterator newblk_start = ei;
		for (; ei != ee && ei->from <= blk.id(); ++ei)
		{
			if (ei->to > blk.id())
			{
				// ... last element is overlapping block borders, so we split it
				splitStart = blk.id()+1;
				splitEnd = ei->to; 
				ei->to = blk.id();
			}
		}

		BooleanBlock::merge( newblk_start, ei, blk, newblk);
		if (splitStart)
		{
			// ... last element is overlapping block borders, no we assign the second half of it to the block
			--ei;
			ei->from = splitStart;
			ei->to = splitEnd;
		}
		if (dbadapter->loadNext( blk))
		{
			// ... is not the last block, so we store it and start with a new one
			dbadapter->store( transaction, newblk);
			newblk.clear();
		}
		else
		{
			if (newblk.full())
			{
				// ... it is not the last block, but full, so we store it and start with a new one
				dbadapter->store( transaction, newblk);
				newblk.clear();
			}
			else
			{
				dbadapter->remove( transaction, newblk.id());
			}
			break;
		}
	}
	if (newblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee && dbadapter->loadLast( blk))
		{
			if (!blk.full())
			{
				dbadapter->remove( transaction, blk.id());
				newblk.swap( blk);
			}
		}
	}
}

