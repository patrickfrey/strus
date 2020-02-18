/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_STRUCTURE_INDEX_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_STRUCTURE_INDEX_MAP_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "structBlock.hpp"
#include "databaseAdapter.hpp"
#include "blockKey.hpp"
#include "private/localStructAllocator.hpp"
#include <vector>
#include <iostream>
#include <set>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

class StructIndexMap
{
public:
	StructIndexMap( DatabaseClientInterface* database_, ErrorBufferInterface* errorhnd_);

	void defineStructureBlock( strus::Index docno, const StructBlock& blk);

	void deleteIndex( strus::Index docno);

	void renameNewDocNumbers( const std::map<strus::Index,strus::Index>& renamemap);
	void getWriteBatch( DatabaseTransactionInterface* transaction);

	void print( std::ostream& out) const;

	void clear();

private:
	typedef std::less<strus::Index> DocnoMapCompare;
	typedef std::map<strus::Index,int,DocnoMapCompare> DocnoMap;

private:
	ErrorBufferInterface* m_errorhnd;		///< error buffer interface
	DatabaseClientInterface* m_database;		///< database client interface
	std::vector<StructBlock> m_blockar;		///< vector of blocks
	DocnoMap m_docnomap;				///< map docno -> int (-1, deleted, else index to m_blockar)
};

}
#endif

