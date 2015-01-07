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
#include "blockKey.hpp"
#include "blockStorage.hpp"
#include "localStructAllocator.hpp"
#include <vector>
#include <iostream>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class PosinfoBlockMap
{
public:
	explicit PosinfoBlockMap( leveldb::DB* db_);
	PosinfoBlockMap( const PosinfoBlockMap& o);

	void definePosinfoPosting(
		const Index& typeno,
		const Index& termno,
		const Index& docno,
		const std::vector<Index>& pos);

	void deletePosinfoPosting(
		const Index& typeno,
		const Index& termno,
		const Index& docno);

	void renameNewTermNumbers( const std::map<Index,Index>& renamemap);

	void getWriteBatch( leveldb::WriteBatch& batch);

private:
	struct Element
	{
		Index docno;
		std::size_t posinfoidx;

		Element( const Element& o)
			:docno(o.docno),posinfoidx(o.posinfoidx){}
		Element( const Index& docno_, const std::size_t& posinfoidx_)
			:docno(docno_),posinfoidx(posinfoidx_){}
	};

	typedef LocalStructAllocator<std::pair<BlockKeyIndex,std::size_t> > MapAllocator;
	typedef std::less<BlockKeyIndex> MapCompare;
	typedef std::map<BlockKeyIndex,std::size_t,MapCompare,MapAllocator> Map;

private:
	static void defineDocnoRangeElement(
			std::vector<BooleanBlock::MergeRange>& docrangear,
			const Index& docno,
			bool isMember);

	void insertNewPosElements(
			BlockStorage<PosinfoBlock>& blkstorage,
			std::vector<Element>::const_iterator& ei,
			const std::vector<Element>::const_iterator& ee,
			PosinfoBlock& newposblk,
			const Index& lastInsertBlockId,
			std::vector<BooleanBlock::MergeRange>& docrangear,
			leveldb::WriteBatch& batch);

	void mergeNewPosElements(
			BlockStorage<PosinfoBlock>& blkstorage,
			std::vector<Element>::const_iterator& ei,
			const std::vector<Element>::const_iterator& ee,
			PosinfoBlock& newposblk,
			std::vector<BooleanBlock::MergeRange>& docrangear,
			leveldb::WriteBatch& batch);

	PosinfoBlock mergePosBlock(
			std::vector<Element>::const_iterator ei,
			const std::vector<Element>::const_iterator& ee,
			const PosinfoBlock& oldblk);

	void clear();

private:
	leveldb::DB* m_db;
	Map m_map;
	std::vector<Element> m_elements;
	std::string m_strings;
	BlockKeyIndex m_lastkey;
};

}
#endif

