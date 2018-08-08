/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_INVERTED_INDEX_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_INVERTED_INDEX_MAP_HPP_INCLUDED
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
#include <set>
#include <map>
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
	void deleteIndex( const Index& docno, const Index& typeno);

	void renameNewNumbers(
			const std::map<Index,Index>& docnoUnknownMap,
			const std::map<Index,Index>& termUnknownMap);

	void getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StatisticsBuilderInterface* statisticsBuilder,
			DocumentFrequencyCache::Batch* dfbatch,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv);

	void print( std::ostream& out) const;

	void clear();

private:
	struct MapKey
	{
		BlockKeyIndex termkey;
		Index docno;

		MapKey( const MapKey& o)
			:termkey(o.termkey),docno(o.docno){}
		MapKey( const Index& typeno_, const Index& termno_, const Index& docno_)
			:termkey(BlockKey(typeno_,termno_).index()),docno(docno_){}

		bool operator < (const MapKey& o) const
		{
			if (termkey < o.termkey) return true;
			if (termkey > o.termkey) return false;
			return docno < o.docno;
		}
	};

	typedef LocalStructAllocator<std::pair<const MapKey,std::size_t> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,std::size_t,MapCompare,MapAllocator> Map;

	typedef InvTermBlock::Element InvTerm;
	typedef std::vector<InvTerm> InvTermList;
	typedef LocalStructAllocator<std::pair<const Index,std::size_t> > InvTermMapAllocator;
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

private:
	DocumentFrequencyMap m_dfmap;
	DatabaseClientInterface* m_database;
	Map m_map;
	std::vector<PosinfoBlock::PositionType> m_posinfo;
	InvTermMap m_invtermmap;
	InvTermList m_invterms;
	Index m_docno;
	std::set<Index> m_docno_deletes;
	std::map<Index, std::set<Index> > m_docno_typeno_deletes;
};

}
#endif

