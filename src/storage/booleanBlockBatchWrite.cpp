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
#include <set>

using namespace strus;

void BooleanBlockBatchWrite::checkBlocks( DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter)
{
	BooleanBlock blk;
	strus::Index prev = 0;

	bool more = dbadapter->loadFirst( blk);
	for (; more; more=dbadapter->loadNext( blk))
	{
		BooleanBlock::NodeCursor cursor;
		strus::Index start = blk.getFirst( cursor);
		if (start < prev) throw strus::runtime_error(_TXT("boolean blocks are overlapping: %d IN [%d,%d]"), (int)prev, (int)start, (int)blk.id());
		prev = blk.id();
	}
}

void BooleanBlockBatchWrite::insertNewElements(
		DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newblk,
		DatabaseTransactionInterface* transaction)
{
	for (;ei != ee; ++ei)
	{
		if ((int)newblk.size() >= Constants::maxBooleanBlockSize())
		{
			if (!newblk.id()) newblk.setId( newblk.getLast());
			dbadapter->store( transaction, newblk);
			newblk.clear();
			newblk.setId( 0);
		}
		if (ei->isMember)
		{
			if (newblk.id() && ei->to > newblk.id())
			{
				dbadapter->remove( transaction, newblk.id());
				newblk.setId( 0);
			}
			newblk.defineRange( ei->from, ei->to - ei->from);
		}
	}
	if (!newblk.empty())
	{
		if (!newblk.id()) newblk.setId( newblk.getLast());
		dbadapter->store( transaction, newblk);
		newblk.clear();
		newblk.setId( 0);
	}
	else if (newblk.id())
	{
		// Remove emptied block
		dbadapter->remove( transaction, newblk.id());
		newblk.setId( 0);
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
	if (ei != ee && !dbadapter->loadUpperBound( ei->from, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "boolean block");

		if (dbadapter->loadLast( blk) && !blk.full())
		{
			dbadapter->remove( transaction, blk.id());
			newblk.swap( blk);
			newblk.setId( 0);
		}
		for (; ei != ee; ++ei)
		{
			if (newblk.full())
			{
				newblk.setId( newblk.getLast());
				// ... block is filled with an acceptable ratio, so we store it, for stopping cascading merges
				dbadapter->store( transaction, newblk);
				newblk.clear();
				newblk.setId( 0);
			}
			newblk.defineRange( ei->from, ei->to - ei->from);
		}
	}
	while (ei != ee && dbadapter->loadUpperBound( ei->from, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "boolean block");

		std::vector<BooleanBlock::MergeRange>::iterator start = ei;
		for (; ei != ee && ei->from <= blk.id(); ++ei)
		{
			if (ei->to > blk.id())
			{
				// ... new element overlapping, replace old block id, means delete old block
				dbadapter->remove( transaction, blk.id());
				blk.setId( ei->to);
				++ei;
				break;
			}
		}
		BooleanBlock::merge( start, ei, blk, newblk);
		while (!newblk.empty() && dbadapter->loadNext( blk))
		{
			if (newblk.filledWithRatio( Constants::minimumBlockFillRatio()))
			{
				// ... it is filled with an acceptable ratio, so we store it, for stopping cascading merges:
				dbadapter->store( transaction, newblk);
				newblk.clear();
				newblk.setId( 0);
				break;
			}
			else
			{
				// Fighting fragmentation by joining blocks:
				start = ei;
				for (; ei != ee && ei->from <= blk.id(); ++ei)
				{
					if (ei->to > blk.id())
					{
						// ... new element overlapping, replace old block id, means delete old block
						dbadapter->remove( transaction, blk.id());
						blk.setId( 0);
						++ei;
						break;
					}
				}
				BooleanBlock newblk_merged = newblk;
				newblk_merged.setId( blk.id());//... gets the id of the follow block
				BooleanBlock::merge_append( start, ei, blk, newblk_merged);
				if (!newblk_merged.filledWithRatio( Constants::minimumBlockFillRatio()))
				{
					// ... block still not big enough, continue joining
					if (newblk.id()) dbadapter->remove( transaction, newblk.id()/*id of the previous block*/);
					newblk.swap( newblk_merged); //... continue with the merged block
				}
				else if (newblk_merged.size() < Constants::maximumBlockFillRatio() * Constants::maxBooleanBlockSize())
				{
					// ... merged block is acceptable, store it
					if (newblk.id()) dbadapter->remove( transaction, newblk.id());//...previous block removed
					newblk_merged.setId( blk.id() ? blk.id() : newblk_merged.getLast());//...id of the follow block appended
					dbadapter->store( transaction, newblk_merged); //... store the merged block
					newblk.clear(); //... reset the processed block, finished merging with follow blocks
					newblk.setId( 0);
				}
				else
				{
					// ... merged block is too big, split it into 2, delete the old one, done
					BooleanBlock newblk_split1;
					BooleanBlock newblk_split2;
					BooleanBlock::split( newblk_merged, newblk_split1, newblk_split2);

					newblk_split1.setId( newblk_split1.getLast()/*new created id inbetween*/);
					newblk_split2.setId( blk.id() ? blk.id() : newblk_merged.getLast());//...id of the follow block appended
					if (newblk.id() && newblk.id() != newblk_split1.id()) dbadapter->remove( transaction, newblk.id());

					dbadapter->store( transaction, newblk_split1);
					dbadapter->store( transaction, newblk_split2);
					newblk.clear();
					newblk.setId( 0);
				}
			}
		}
	}
}

