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
#include "blockMap.hpp"
#include "blockKey.hpp"
#include <vector>
#include <iostream>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class PosinfoBlockMap
	:protected BlockMap<PosinfoBlock,PosinfoBlockElement>
{
public:
	typedef BlockMap<PosinfoBlock,PosinfoBlockElement> Parent;

public:
	PosinfoBlockMap( leveldb::DB* db_)
		:BlockMap<PosinfoBlock,PosinfoBlockElement>(db_){}
	PosinfoBlockMap( const PosinfoBlockMap& o)
		:BlockMap<PosinfoBlock,PosinfoBlockElement>(o){}

	void definePosinfoPosting(
		const Index& typeno,
		const Index& termno,
		const Index& docno,
		const std::vector<Index>& pos)
	{
		defineElement( BlockKey( typeno, termno), docno, pos);
	}

	void deletePosinfoPosting(
		const Index& typeno,
		const Index& termno,
		const Index& docno)
	{
		deleteElement( BlockKey( typeno, termno), docno);
	}
	
	void getWriteBatch( leveldb::WriteBatch& batch)
	{
		getWriteBatchMerge( batch);
	}
};

}
#endif

