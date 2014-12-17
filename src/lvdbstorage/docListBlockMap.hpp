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
#ifndef _STRUS_LVDB_DOCLIST_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_DOCLIST_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "booleanBlock.hpp"
#include "blockKey.hpp"
#include "blockStorage.hpp"
#include <cstdlib>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class DocListBlockMap
{
public:
	explicit DocListBlockMap( leveldb::DB* db_)
		:m_db(db_){}
	DocListBlockMap( const DocListBlockMap& o)
		:m_db(o.m_db),m_map(o.m_map){}

	void definePosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno);

	void deletePosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch( leveldb::WriteBatch& batch);

private:
	void insertNewElements(
			BlockStorage<BooleanBlock>& blkstorage,
			BooleanBlockElementMap::const_iterator& ei,
			const BooleanBlockElementMap::const_iterator& ee,
			BooleanBlock& newblk,
			const Index& lastInsertBlockId,
			leveldb::WriteBatch& batch);

	void mergeNewElements(
			BlockStorage<BooleanBlock>& blkstorage,
			BooleanBlockElementMap::const_iterator& ei,
			const typename BooleanBlockElementMap::const_iterator& ee,
			BooleanBlock& newblk,
			leveldb::WriteBatch& batch);

	void markSetElement(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno,
		bool isMember);

private:
	typedef std::map<BlockKeyIndex,BooleanBlockElementMap> Map;

private:
	leveldb::DB* m_db;
	Map m_map;
};

}
#endif

