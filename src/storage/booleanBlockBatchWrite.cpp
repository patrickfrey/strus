/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
		for (std::size_t mm = newblk.size(); ei != ee && (int)mm < Constants::maxBooleanBlockSize(); ++ei)
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

