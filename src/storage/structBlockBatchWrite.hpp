/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_STRUCTURE_BLOCK_BATCH_WRITE_HPP_INCLUDED
#define _STRUS_STORAGE_STRUCTURE_BLOCK_BATCH_WRITE_HPP_INCLUDED
#include "strus/index.hpp"
#include "structBlock.hpp"
#include "databaseAdapter.hpp"
#include "strus/reference.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include <vector>

namespace strus {

/// \class StructBlockBatchWrite
/// \brief Methods for merging and inserting structure blocks
class StructBlockBatchWrite
{
public:
	static void checkBlocks( DatabaseAdapter_StructBlock::WriteCursor* dbadapter);
	
	static void insertNewElements(
			DatabaseAdapter_StructBlock::WriteCursor* dbadapter,
			std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ei,
			const std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ee,
			StructBlockBuilder& newblk,
			DatabaseTransactionInterface* transaction);

	static void mergeNewElements(
			DatabaseAdapter_StructBlock::WriteCursor* dbadapter,
			std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ei,
			const std::vector<StructBlockBuilder::StructDeclaration>::const_iterator& ee,
			StructBlockBuilder& newblk,
			DatabaseTransactionInterface* transaction);
};

}//namespace
#endif


