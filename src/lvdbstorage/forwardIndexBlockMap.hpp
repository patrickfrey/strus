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
#ifndef _STRUS_LVDB_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_FORWARD_INDEX_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "forwardIndexBlock.hpp"
#include "blockKey.hpp"
#include "blockStorage.hpp"
#include "localStructAllocator.hpp"
#include <vector>
#include <map>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class ForwardIndexBlockMap
{
public:
	explicit ForwardIndexBlockMap( leveldb::DB* db_)
		:m_db(db_){}
	ForwardIndexBlockMap( const ForwardIndexBlockMap& o)
		:m_db(o.m_db),m_map(o.m_map){}

	void defineForwardIndexTerm(
		const Index& typeno,
		const Index& docno,
		const Index& pos,
		const std::string& termstring);

	void getWriteBatch( leveldb::WriteBatch& batch);


	template <class TermnoMap>
	std::map<Index,Index> getTermOccurrencies(
		const Index& typeno,
		const Index& docno,
		TermnoMap& elementmap)
	{
		std::map<Index,Index> rt;
		getElementOccurrencies<TermnoMap>(
			rt, BlockKey( typeno, docno), elementmap);
		return rt;
	}

private:
	template <class TermnoMap>
	void getElementOccurrencies(
		std::map<Index,Index>& result,
		const BlockKey& dbkey,
		TermnoMap& map)
	{
		BlockStorage<ForwardIndexBlock> blkstorage(
			m_db, DatabaseKey::ForwardIndexPrefix, dbkey, false);

		Index blkidx = 0;
		const ForwardIndexBlock* blk = blkstorage.load( blkidx);

		for (; blk; blk = blkstorage.load( blkidx))
		{
			blkidx = blk->id()+1;
			ForwardIndexBlock::const_iterator bi = blk->begin(), be = blk->end();
			for (; bi != be; ++bi)
			{
				result[ map( *bi)] += 1;
			}
		}
	}

private:
	typedef std::pair<ForwardIndexBlock,std::size_t> BlockListElem;
	typedef std::vector<BlockListElem> BlockList;

	typedef LocalStructAllocator<std::pair<BlockKeyIndex,std::size_t> > MapAllocator;
	typedef std::less<BlockKeyIndex> MapCompare;
	typedef std::map<BlockKeyIndex,std::size_t,MapCompare,MapAllocator> Map;

private:
	enum {MaxBlockId=1<<30};
	leveldb::DB* m_db;
	BlockList m_blockar;
	Map m_map;
};

}
#endif

