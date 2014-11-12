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
#include <cstdlib>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>

namespace strus {

class DocnoBlockMap
{
public:
	DocnoBlockMap( leveldb::DB* db_)
		:m_db(db_),m_itr(0){}
	DocnoBlockMap( const DocnoBlockMap& o)
		:m_db(o.m_db),m_itr(0),m_map(o.m_map){}

	void defineDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno,
		unsigned int ff,
		float weight);

	void flush();

private:
	typedef std::map<Index,DocnoBlock::Element> ElementMap;

	struct Term
	{
		Index type;
		Index value;

		Term( const Index& t, const Index& v)
			:type(t),value(v){}
		Term( const Term& o)
			:type(o.type),value(o.value){}

		bool operator < (const Term& o) const
		{
			if (type < o.type) return true;
			if (type > o.type) return false;
			return value < o.value;
		}
	};

	void writeBlock(
		leveldb::WriteBatch& batch,
		const Index& typeno,
		const Index& valueno,
		const DocnoBlock::Element* blkptr,
		std::size_t blksize);

	void deleteBlock(
		leveldb::WriteBatch& batch,
		const Index& typeno,
		const Index& valueno,
		const Index& docno);

	void writeMergeBlock(
		leveldb::WriteBatch& batch,
		const Index& typeno,
		const Index& valueno,
		ElementMap::const_iterator& ei,
		const ElementMap::const_iterator& ee,
		const DocnoBlock* blk);

private:
	typedef std::map<Term,ElementMap> Map;
	enum {BlockSize=128};

private:
	leveldb::DB* m_db;
	leveldb::Iterator* m_itr;
	boost::mutex m_mutex;
	Map m_map;
};

}
#endif

