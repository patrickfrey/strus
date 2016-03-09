/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_STORAGE_BOOLEAN_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_BOOLEAN_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
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
	static void insertNewElements(
			DatabaseAdapter_BooleanBlock::WriteCursor* dbadapter,
			std::vector<BooleanBlock::MergeRange>::iterator& ei,
			const std::vector<BooleanBlock::MergeRange>::iterator& ee,
			BooleanBlock& newblk,
			const Index& lastInsertBlockId,
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


