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
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class PosinfoBlockMap
{
public:
	PosinfoBlockMap( leveldb::DB* db_)
		:m_db(db_),m_itr(0){}
	PosinfoBlockMap( const PosinfoBlockMap& o)
		:m_db(o.m_db),m_itr(0),m_map(o.m_map){}

	void definePosinfo(
		const Index& docno,
		const std::vector<Index>& pos);

	void deletePosinfo(
		const Index& docno);
	
	void getWriteBatch( leveldb::WriteBatch& batch);

private:
	typedef std::map<Index,std::vector<Index> > Map;

	void writeBlock(
		leveldb::WriteBatch& batch,
		const Index& docno,
		const PosinfoBlock& blk);

	void deleteBlock(
		leveldb::WriteBatch& batch,
		const Index& docno);

	void writeMergeBlock(
		leveldb::WriteBatch& batch,
		const Index& typeno,
		const Index& valueno,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		const PosinfoBlock& blk);

private:
	enum {BlockSize=1024};

private:
	leveldb::DB* m_db;
	leveldb::Iterator* m_itr;
	boost::mutex m_mutex;
	Map m_map;
};

}
#endif

