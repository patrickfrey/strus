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
#ifndef _STRUS_LVDB_DOCNO_ITERATOR_HPP_INCLUDED
#define _STRUS_LVDB_DOCNO_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "databaseKey.hpp"
#include "blockStorage.hpp"
#include "floatConversions.hpp"
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <leveldb/db.h>

namespace strus {

class DocnoIterator
{
public:
	DocnoIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno)
		:m_blockReader( db_, DocnoBlock::databaseKey( termtypeno, termvalueno), true){}
	DocnoIterator( const DocnoIterator& o)
		:m_blockReader(o.m_blockReader){}

	~DocnoIterator(){}

	Index skipDoc( const Index& docno_);

	unsigned int frequency()
	{
		return m_curelem.initialized()?m_curelem->ff():0;
	}

	Index docno() const
	{
		return m_curelem.initialized()?m_curelem->docno():0;
	}

	float weight() const
	{
		return m_curelem.initialized()?m_curelem->weight():0.0;
	}

private:
	BlockStorage<DocnoBlock> m_blockReader;
	DocnoBlock::const_iterator m_curelem;
};

}
#endif

