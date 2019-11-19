/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ffBlockBatchWrite.hpp"
#include "databaseAdapter.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "private/internationalization.hpp"
#include <set>

using namespace strus;

void FfBlockBatchWrite::insertNewElements(
		DatabaseAdapter_FfBlock::WriteCursor* dbadapter,
		std::vector<FfBlockBuilder::FfDeclaration>::iterator& ei,
		const std::vector<FfBlockBuilder::FfDeclaration>::iterator& ee,
		FfBlockBuilder& newblk,
		DatabaseTransactionInterface* transaction)
{
	for (;ei != ee; ++ei)
	{
		if ((int)newblk.size() >= Constants::maxFfBlockSize())
		{
			if (!newblk.id()) newblk.setId( newblk.lastDoc());
			dbadapter->store( transaction, newblk.createBlock());
			newblk.clear();
		}
		if (ei->ff)
		{
			if (newblk.id() && ei->docno > newblk.id())
			{
				dbadapter->remove( transaction, newblk.id());
				newblk.setId( 0);
			}
			newblk.append( ei->docno, ei->ff);
		}
	}
	if (!newblk.empty())
	{
		if (!newblk.id()) newblk.setId( newblk.lastDoc());
		dbadapter->store( transaction, newblk.createBlock());
		newblk.clear();
	}
}

void FfBlockBatchWrite::mergeNewElements(
		DatabaseAdapter_FfBlock::WriteCursor* dbadapter,
		std::vector<FfBlockBuilder::FfDeclaration>::iterator& ei,
		const std::vector<FfBlockBuilder::FfDeclaration>::iterator& ee,
		FfBlockBuilder& newblk,
		DatabaseTransactionInterface* transaction)
{
	FfBlock blk;
	if (ei != ee && !dbadapter->loadUpperBound( ei->docno, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "ff block");

		if (dbadapter->loadLast( blk) && !blk.full())
		{
			newblk = FfBlockBuilder( blk);
			dbadapter->remove( transaction, blk.id());
			newblk.setId(0);
		}
		for (; ei != ee; ++ei)
		{
			if (newblk.full())
			{
				// ... block is filled with an acceptable ratio, so we store it, for stopping cascading merges
				dbadapter->store( transaction, newblk.createBlock());
				newblk.clear();
			}
			if (ei->ff)
			{
				newblk.append( ei->docno, ei->ff);
			}
		}
	}
	while (ei != ee && dbadapter->loadUpperBound( ei->docno, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "ff block");

		std::vector<FfBlockBuilder::FfDeclaration>::iterator start = ei;
		for (; ei != ee && ei->docno <= blk.id(); ++ei){}

		FfBlockBuilder::merge( start, ei, blk, newblk);
		while (!newblk.empty() && dbadapter->loadNext( blk))
		{
			if (newblk.filledWithRatio( Constants::minimumBlockFillRatio()))
			{
				// ... it is filled with an acceptable ratio, so we store it, for stopping cascading merges:
				dbadapter->store( transaction, newblk.createBlock());
				newblk.clear();
				break;
			}
			else
			{
				// Fighting fragmentation by joining blocks:
				start = ei;
				for (; ei != ee && ei->docno <= blk.id(); ++ei){}

				FfBlockBuilder newblk_merged = newblk;
				newblk_merged.setId( 0);
				FfBlockBuilder::merge_append( start, ei, blk, newblk_merged);
				if (!newblk_merged.filledWithRatio( Constants::minimumBlockFillRatio()))
				{
					// ... block still not big enough, continue joining
					if (newblk.id()) dbadapter->remove( transaction, newblk.id());
					newblk.swap( newblk_merged);
					newblk.setId( blk.id());
				}
				else if (newblk_merged.size() < Constants::maximumBlockFillRatio() * Constants::maxFfBlockSize())
				{
					// ... merged block is acceptable, store it
					if (newblk.id()) dbadapter->remove( transaction, newblk.id());
					newblk_merged.setId( blk.id());
					dbadapter->store( transaction, newblk_merged.createBlock());
					newblk.clear();
				}
				else
				{
					// ... merged block is too big, split it into 2, delete the old one, done
					FfBlockBuilder newblk_split1;
					FfBlockBuilder newblk_split2;
					FfBlockBuilder::split( newblk_merged, newblk_split1, newblk_split2);

					newblk_split1.setId( newblk_split1.lastDoc());
					newblk_split2.setId( blk.id());
					dbadapter->store( transaction, newblk_split1.createBlock());
					dbadapter->store( transaction, newblk_split2.createBlock());
					newblk.clear();
				}
			}
		}
	}
}

