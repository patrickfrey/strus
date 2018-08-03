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
	explicit StructIndexMap( DatabaseClientInterface* database_);

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
	/// \brief Key of the structure map
	/// \note The key includes source and sink to have the correct order and because they must not overlap
	struct MapKey
	{
		Index structno;
		Index docno;
		Index source_end;
		Index sink_end;

		MapKey( const MapKey& o)
			:structno(o.structno),docno(o.docno),source_end(o.source_end),sink_end(o.sink_end){}
		MapKey( const Index& structno_, const Index& docno_, Index source_end_, Index sink_end_)
			:structno(structno_),docno(docno_),source_end(source_end_),sink_end(sink_end_){}

		bool operator < (const MapKey& o) const
		{
			if (structno < o.structno) return true;
			if (structno > o.structno) return false;
			if (docno < o.docno) return true;
			if (docno > o.docno) return false;
			if (source_end < o.source_end) return true;
			if (source_end > o.source_end) return false;
			return (sink_end < o.sink_end);
		}
	};

	struct MapValue
	{
		Index source_start;
		Index sink_start;

		MapValue()
			:source_start(0),sink_start(0){}
		MapValue( Index source_start_, Index sink_start_)
			:source_start(source_start_),sink_start(sink_start_){}
		MapValue( const MapValue& o)
			:source_start(o.source_start),sink_start(o.sink_start){}
	};

	struct StructDocRel
	{
		Index structno;
		Index docno;

		StructDocRel()
			:structno(0),docno(0){}
		StructDocRel( Index docno_, Index structno_)
			:structno(structno_),docno(docno_){}
		StructDocRel( const StructDocRel& o)
			:structno(o.structno),docno(o.docno){}

		bool operator < (const StructDocRel& o) const
		{
			if (structno < o.structno) return true;
			if (structno > o.structno) return false;
			return (docno < o.docno);
		}
	};

	typedef LocalStructAllocator<std::pair<const MapKey,std::size_t> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,MapValue,MapCompare,MapAllocator> Map;

private:
	void insertNewElements(
			DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct,
			DatabaseTransactionInterface* transaction,
			Map::const_iterator& ei,
			const Map::const_iterator& ee,
			StructBlockBuilder& newblk,
			std::vector<BooleanBlock::MergeRange>& docrangear);

	void mergeNewElements(
			DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct,
			DatabaseTransactionInterface* transaction,
			Map::const_iterator& ei,
			const Map::const_iterator& ee,
			StructBlockBuilder& newblk,
			std::vector<BooleanBlock::MergeRange>& docrangear);

	void mergeBlock(
			DatabaseAdapter_StructBlock::WriteCursor& dbadapter_struct, 
			DatabaseTransactionInterface* transaction,
			Map::const_iterator ei,
			const Map::const_iterator& ee,
			const StructBlock& oldblk,
			StructBlockBuilder& newblk);

private:
	DatabaseClientInterface* m_database;
	Map m_map;
	Index m_docno;
	std::set<Index> m_structnoset;
	std::set<Index> m_docno_deletes;
	std::set<StructDocRel> m_structno_docno_deletes;
};

}
#endif

