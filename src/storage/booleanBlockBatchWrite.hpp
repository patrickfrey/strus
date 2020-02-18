/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BOOLEAN_BLOCK_BATCH_WRITE_HPP_INCLUDED
#define _STRUS_STORAGE_BOOLEAN_BLOCK_BATCH_WRITE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "booleanBlock.hpp"
#include "databaseAdapter.hpp"
#include "strus/reference.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include <vector>

namespace strus {

/// \class BooleanBlockBatchWrite
/// \brief Methods for merging and inserting boolean blocks
class BooleanBlockBatchWrite
{
public:
	static void checkBlocks( DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter);

	static void insertNewElements(
			DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
			std::vector<BooleanBlock::MergeRange>::iterator& ei,
			const std::vector<BooleanBlock::MergeRange>::iterator& ee,
			BooleanBlock& newblk,
			DatabaseTransactionInterface* transaction);

	static void mergeNewElements(
			DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
			std::vector<BooleanBlock::MergeRange>::iterator& ei,
			const std::vector<BooleanBlock::MergeRange>::iterator& ee,
			BooleanBlock& newblk,
			DatabaseTransactionInterface* transaction);
};

}//namespace
#endif


