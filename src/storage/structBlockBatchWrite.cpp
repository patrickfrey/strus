/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structBlockBatchWrite.hpp"
#include "databaseAdapter.hpp"
#include "strus/databaseClientInterface.hpp"
#include "private/internationalization.hpp"
#include <set>
using namespace strus;

void StructBlockBatchWrite::checkBlocks( DatabaseAdapter_StructBlock::WriteCursor* dbadapter)
{
	StructBlock blk;
	strus::Index prev = 0;
	bool more = dbadapter->loadFirst( blk);
	for (; more; more=dbadapter->loadNext( blk))
	{
		DocIndexNodeCursor cursor;
		strus::Index start = blk.firstDoc( cursor);
		if (start < prev) throw strus::runtime_error(_TXT("structure blocks are overlapping: %d IN [%d,%d]"), (int)prev, (int)start, (int)blk.id());
		prev = blk.id();
	}
}

void StructBlockBatchWrite::insertNewElements(
		DatabaseAdapter_StructBlock::WriteCursor* dbadapter,
		std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ei,
		const std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ee,
		StructBlockBuilder& newblk,
		DatabaseTransactionInterface* transaction)
{
	while (ei != ee)
	{
		std::vector<StructBlockBuilder::StructDeclaration>::const_iterator
			en = ei;
		for (++en; en != ee && en->docno == ei->docno; ++en){}
		if (!newblk.fitsInto( en-ei) && !newblk.empty())
		{
			if (!newblk.id()) newblk.setId( newblk.lastDoc());
			dbadapter->store( transaction, newblk.createBlock());
			newblk.clear();
		}
		if (newblk.id() && ei->docno > newblk.id())
		{
			dbadapter->remove( transaction, newblk.id());
			newblk.setId( 0);
		}
		for (; ei != en; ++ei)
		{
			if (ei->src.defined() || ei->sink.defined())
			{
				newblk.append( ei->docno, ei->src, ei->sink);
			}
		}
	}
	if (!newblk.empty())
	{
		if (!newblk.id()) newblk.setId( newblk.lastDoc());
		dbadapter->store( transaction, newblk.createBlock());
		newblk.clear();
	}
}

void StructBlockBatchWrite::mergeNewElements(
		DatabaseAdapter_StructBlock::WriteCursor* dbadapter,
		std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ei,
		const std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ee,
		StructBlockBuilder& newblk,
		DatabaseTransactionInterface* transaction)
{
	StructBlock blk;
	if (ei != ee && !dbadapter->loadUpperBound( ei->docno, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "structure block");

		if (dbadapter->loadLast( blk) && !blk.full())
		{
			newblk = StructBlockBuilder( blk);
			dbadapter->remove( transaction, blk.id());
			newblk.setId(0);
		}
		while (ei != ee)
		{
			std::vector<StructBlockBuilder::StructDeclaration>::const_iterator
				en = ei;
			for (++en; en != ee && en->docno == ei->docno; ++en){}
			if (!newblk.fitsInto( en-ei) && !newblk.empty())
			{
				// ... block is filled with an acceptable ratio, so we store it, for stopping cascading merges
				dbadapter->store( transaction, newblk.createBlock());
				newblk.clear();
			}
			for (; ei != en; ++ei)
			{
				if (ei->src.defined() || ei->sink.defined())
				{
					newblk.append( ei->docno, ei->src, ei->sink);
				}
			}
		}
	}
	while (ei != ee && dbadapter->loadUpperBound( ei->docno, blk))
	{
		if (!newblk.empty()) throw strus::runtime_error(_TXT("logic error: unexpected %s upperbound exists with block not cleared"), "structure block");

		std::vector<StructBlockBuilder::StructDeclaration>::const_iterator start = ei;
		for (; ei != ee && ei->docno <= blk.id(); ++ei){}

		StructBlockBuilder::merge( start, ei, blk, newblk);
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

				StructBlockBuilder newblk_merged = newblk;
				newblk_merged.setId( 0);
				StructBlockBuilder::merge_append( start, ei, blk, newblk_merged);
				if (!newblk_merged.filledWithRatio( Constants::minimumBlockFillRatio()))
				{
					// ... block still not big enough, continue joining
					if (newblk.id()) dbadapter->remove( transaction, newblk.id());
					newblk.swap( newblk_merged);
					newblk.setId( blk.id());
				}
				else if (newblk_merged.firstDoc() == newblk_merged.lastDoc()
					|| newblk_merged.size() < Constants::maximumBlockFillRatio() * Constants::maxStructBlockSize())
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
					StructBlockBuilder newblk_split1;
					StructBlockBuilder newblk_split2;
					StructBlockBuilder::split( newblk_merged, newblk_split1, newblk_split2);

					newblk_split1.setId( newblk_split1.lastDoc());
					newblk_split2.setId( blk.id());
					if (newblk.id() && newblk.id() != newblk_split1.id()) dbadapter->remove( transaction, newblk.id());

					dbadapter->store( transaction, newblk_split1.createBlock());
					dbadapter->store( transaction, newblk_split2.createBlock());
					newblk.clear();
				}
			}
		}
	}
}

