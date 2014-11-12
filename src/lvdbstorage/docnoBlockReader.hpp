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
#ifndef _STRUS_LVDB_METADATA_DOCNOBLOCK_READER_HPP_INCLUDED
#define _STRUS_LVDB_METADATA_DOCNOBLOCK_HPP_INCLUDED
#include "strus/index.hpp"
#include "docnoBlock.hpp"
#include "databaseKey.hpp"
#include <leveldb/db.h>

namespace strus {

class DocnoBlockReader
{
public:
	DocnoBlockReader( leveldb::DB* db_, Index typeno_, Index termno_);
	DocnoBlockReader( const DocnoBlockReader& o);
	~DocnoBlockReader();

	const DocnoBlock* readBlock( const Index& docno_);

private:
	leveldb::DB* m_db;
	leveldb::Iterator* m_itr;
	DatabaseKey m_key;
	std::size_t m_keysize;
	Index m_last_docno;
	DocnoBlock m_docnoBlock;
};

}
#endif

