/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "forwardIndexBlock.hpp"
#include "blockKey.hpp"
#include "private/localStructAllocator.hpp"
#include "private/stringMap.hpp"
#include <vector>
#include <map>
#include <set>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class ForwardIndexMap
{
public:
	ForwardIndexMap( DatabaseClientInterface* database_, Index maxtype_, unsigned int maxblocksize_=ForwardIndexBlock::MaxBlockTokens)
		:m_database(database_),m_docno(0),m_maxtype(maxtype_),m_maxblocksize(maxblocksize_){}

	void openForwardIndexDocument( const Index& docno);

	void defineForwardIndexTerm(
		const Index& typeno,
		const Index& pos,
		const std::string& termstring);

	void closeForwardIndexDocument();

	void deleteIndex( const Index& docno);
	void deleteIndex( const Index& docno, const Index& typeno);

	void renameNewDocNumbers( const std::map<Index,Index>& renamemap);
	void getWriteBatch( DatabaseTransactionInterface* transaction);

private:
	void clear();

private:
	struct MapKey
	{
		BlockKeyIndex termkey;
		Index maxpos;

		MapKey( const MapKey& o)
			:termkey(o.termkey),maxpos(o.maxpos){}
		MapKey( const Index& typeno_, const Index& docno_, const std::size_t& maxpos_)
			:termkey(BlockKey(typeno_,docno_).index()),maxpos(maxpos_){}

		bool operator < (const MapKey& o) const
		{
			if (termkey < o.termkey) return true;
			if (termkey > o.termkey) return false;
			return maxpos < o.maxpos;
		}
	};

	typedef LocalStructAllocator<std::pair<MapKey,std::size_t> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,std::size_t,MapCompare,MapAllocator> Map;
	typedef std::vector<ForwardIndexBlock> BlockList;

	typedef std::pair<Index,const char*> CurblockElem;
	typedef std::vector<CurblockElem> CurblockElemList;
	typedef LocalStructAllocator<std::pair<Index,CurblockElemList> > CurblockMapAllocator;
	typedef std::map<Index,CurblockElemList,std::less<Index>,CurblockMapAllocator> CurblockMap;

private:
	void closeCurblock( const Index& typeno, CurblockElemList& blk);
	void closeCurblocks();

private:
	DatabaseClientInterface* m_database;
	Map m_map;
	BlockList m_blocklist;
	CurblockMap m_curblockmap;
	StringVector m_strings;
	Index m_docno;
	Index m_maxtype;
	unsigned int m_maxblocksize;
	std::set<Index> m_docno_deletes;
	std::map<Index, std::set<Index> > m_docno_typeno_deletes;
};

}
#endif

