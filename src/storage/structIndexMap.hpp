/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_STRUCTURE_INDEX_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_STRUCTURE_INDEX_MAP_HPP_INCLUDED
#include "strus/index.hpp"
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

class StructIndexMap
{
public:
	StructIndexMap( DatabaseClientInterface* database_, Index maxstructno_);

	void defineStructure(
		const Index& structno,
		const Index& docno,
		const IndexRange& source,
		const IndexRange& sink);

	void deleteIndex( const Index& docno);
	void deleteIndex( const Index& docno, const Index& structno);

	void renameNewNumbers( const std::map<Index,Index>& docnoUnknownMap);

	void getWriteBatch( DatabaseTransactionInterface* transaction);

	void print( std::ostream& out) const;

	void clear();

private:
	struct StructDef
	{
		IndexRange source;
		IndexRange sink;

		StructDef( const IndexRange& source_, const IndexRange& sink_)
			:source(source_),sink(sink_){}
		StructDef( const StructDef& o)
			:source(o.source),sink(o.sink){}

		bool operator < (const StructDef& o) const
		{
			if (source.end() < o.source.end()) return true;
			if (source.end() > o.source.end()) return false;
			if (sink.end() < o.sink.end()) return true;
			if (sink.end() > o.sink.end()) return false;
			if (source.start() < o.source.start()) return true;
			if (source.start() > o.source.start()) return false;
			return (sink.start() < o.sink.start());
		}
	};

	typedef LocalStructAllocator<StructDef> StructDefAllocator;
	typedef std::less<StructDef> StructDefCompare;
	typedef std::set<StructDef,StructDefCompare,StructDefAllocator> StructDefSet;

	typedef LocalStructAllocator<std::pair<Index,int> > MapAllocator;
	typedef std::less<Index> MapCompare;
	typedef std::map<Index,int,MapCompare,MapAllocator> Map;

private:
	void deleteInsertedStructs( const Index& docno, const Index& structno);
	void loadStoredElementsFromBlock( const Index& structno, const StructBlock& blk);
	void writeNewBlocks(
			DatabaseAdapter_StructBlock::WriteCursor& dbadapter,
			DatabaseTransactionInterface* transaction,
			Map::const_iterator mi,
			const Map::const_iterator& me,
			StructBlockBuilder& blk);
	bool fitsNofStructuresLeft( Map::const_iterator mi, const Map::const_iterator& me, int maxLimit) const;

private:
	DatabaseClientInterface* m_database;		///< database client interface
	std::vector<StructDefSet> m_defar;		///< vector doc index index -> structures
	std::vector<Map> m_mapar;			///< map docno -> doc index or -1 (deleted)
	Index m_docno;					///< current document number
};

}
#endif

