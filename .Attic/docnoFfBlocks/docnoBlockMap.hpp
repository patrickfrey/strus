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
#ifndef _STRUS_LVDB_DOCNO_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_DOCNO_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "docnoBlock.hpp"
#include "blockKey.hpp"
#include "blockStorage.hpp"
#include "localStructAllocator.hpp"
#include <cstdlib>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class DocnoBlockMap
{
public:
	explicit DocnoBlockMap( leveldb::DB* db_)
		:m_db(db_){}
	DocnoBlockMap( const DocnoBlockMap& o)
		:m_db(o.m_db),m_map(o.m_map){}

	void defineDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno,
		unsigned int ff,
		float weight);

	void deleteDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch( leveldb::WriteBatch& batch);
	
private:
	void insertNewElements(
			BlockStorage<DocnoBlock>& blkstorage,
			DocnoBlockElementMap::const_iterator& ei,
			const DocnoBlockElementMap::const_iterator& ee,
			DocnoBlock& newblk,
			const Index& lastInsertBlockId,
			leveldb::WriteBatch& batch);

	void mergeNewElements(
			BlockStorage<DocnoBlock>& blkstorage,
			DocnoBlockElementMap::const_iterator& ei,
			const typename DocnoBlockElementMap::const_iterator& ee,
			DocnoBlock& newblk,
			leveldb::WriteBatch& batch);
	
private:
	typedef LocalStructAllocator<std::pair<BlockKeyIndex,DocnoBlockElementMap> > MapAllocator;
	typedef std::less<BlockKeyIndex> MapCompare;
	typedef std::map<BlockKeyIndex,DocnoBlockElementMap,MapCompare,MapAllocator> Map;

private:
	leveldb::DB* m_db;
	Map m_map;
};

}
#endif

