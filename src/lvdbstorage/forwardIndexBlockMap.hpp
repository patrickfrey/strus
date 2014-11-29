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
#include "blockMap.hpp"
#include "blockKey.hpp"
#include <vector>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
/*[-]*/#include <iostream>

namespace strus {

typedef std::string ForwardIndexBlockElement;

class ForwardIndexBlockMap
	:protected BlockMap<ForwardIndexBlock,ForwardIndexBlockElement>
{
public:
	ForwardIndexBlockMap( leveldb::DB* db_)
		:BlockMap<ForwardIndexBlock,ForwardIndexBlockElement>(db_){}
	ForwardIndexBlockMap( const ForwardIndexBlockMap& o)
		:BlockMap<ForwardIndexBlock,ForwardIndexBlockElement>(o){}

	void defineForwardIndexTerm(
		const Index& typeno,
		const Index& docno,
		const Index& pos,
		const std::string& termstring)
	{
		//[-] std::cout << "Forward index term typeno "  << typeno << " docno " << docno << " pos " << pos << " value " << termstring << std::endl;
		defineElement( BlockKey( typeno, docno), pos, termstring);
	}

	void deleteForwardIndexTerm(
		const Index& typeno,
		const Index& docno,
		const Index& pos)
	{
		deleteElement( BlockKey( typeno, docno), pos);
	}

	void getWriteBatch( leveldb::WriteBatch& batch)
	{
		getWriteBatchReplace( batch);
	}

	template <class TermnoMap>
	std::map<Index,Index> getTermOccurrencies(
		const Index& typeno,
		const Index& docno,
		TermnoMap& elementmap)
	{
		std::map<Index,Index> rt;
		getElementOccurrencies<TermnoMap, Index>(
			rt, BlockKey( typeno, docno), elementmap);
		return rt;
	}
};

}
#endif

