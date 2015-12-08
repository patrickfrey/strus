/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_LVDB_POSINFO_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_POSINFO_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "posinfoBlock.hpp"
#include "booleanBlock.hpp"
#include "invTermBlock.hpp"
#include "documentFrequencyMap.hpp"
#include "documentFrequencyCache.hpp"
#include "databaseAdapter.hpp"
#include "blockKey.hpp"
#include "private/localStructAllocator.hpp"
#include <vector>
#include <iostream>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class InvertedIndexMap
{
public:
	explicit InvertedIndexMap( DatabaseClientInterface* database_);

	void definePosinfoPosting(
		const Index& typeno,
		const Index& termno,
		const Index& docno,
		const std::vector<Index>& pos);

	void deleteIndex( const Index& docno);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StatisticsBuilderInterface* statisticsBuilder,
			DocumentFrequencyCache::Batch* dfbatch,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv);

private:
	struct MapKey
	{
		BlockKeyIndex termkey;
		Index docno;

		MapKey( const MapKey& o)
			:termkey(o.termkey),docno(o.docno){}
		MapKey( const Index& typeno_, const Index& termno_, const std::size_t& docno_)
			:termkey(BlockKey(typeno_,termno_).index()),docno(docno_){}

		bool operator < (const MapKey& o) const
		{
			if (termkey < o.termkey) return true;
			if (termkey > o.termkey) return false;
			return docno < o.docno;
		}
	};

	typedef LocalStructAllocator<std::pair<MapKey,std::size_t> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,std::size_t,MapCompare,MapAllocator> Map;

	typedef InvTermBlock::Element InvTerm;
	typedef std::vector<InvTerm> InvTermList;
	typedef LocalStructAllocator<std::pair<Index,std::size_t> > InvTermMapAllocator;
	typedef std::less<Index> InvTermMapCompare;
	typedef std::map<Index,std::size_t,InvTermMapCompare,InvTermMapAllocator> InvTermMap;

private:
	static void defineDocnoRangeElement(
			std::vector<BooleanBlock::MergeRange>& docrangear,
			const Index& docno,
			bool isMember);

	void insertNewPosElements(
			DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
			DatabaseTransactionInterface* transaction,
			Map::const_iterator& ei,
			const Map::const_iterator& ee,
			PosinfoBlockBuilder& newposblk,
			std::vector<BooleanBlock::MergeRange>& docrangear);

	void mergeNewPosElements(
			DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
			DatabaseTransactionInterface* transaction,
			Map::const_iterator& ei,
			const Map::const_iterator& ee,
			PosinfoBlockBuilder& newposblk,
			std::vector<BooleanBlock::MergeRange>& docrangear);

	void mergePosBlock(
			DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo, 
			DatabaseTransactionInterface* transaction,
			Map::const_iterator ei,
			const Map::const_iterator& ee,
			const PosinfoBlock& oldblk,
			PosinfoBlockBuilder& newblk);

	void clear();

private:
	DocumentFrequencyMap m_dfmap;
	DatabaseClientInterface* m_database;
	Map m_map;
	std::vector<PosinfoBlock::PositionType> m_posinfo;
	InvTermMap m_invtermmap;
	InvTermList m_invterms;
	Index m_docno;
	std::vector<Index> m_deletes;
};

}
#endif

