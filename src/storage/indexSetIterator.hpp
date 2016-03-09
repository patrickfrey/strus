/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_STORAGE_INDEX_SET_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_INDEX_SET_ITERATOR_HPP_INCLUDED
#include "booleanBlock.hpp"
#include "databaseAdapter.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;

class IndexSetIterator
{
public:
	IndexSetIterator(
			const DatabaseClientInterface* database_,
			DatabaseKey::KeyPrefix dbprefix_,
			const BlockKey& key_,
			bool useCache_);
	~IndexSetIterator(){}

	Index skip( const Index& elemno_);
	Index elemno() const			{return m_elemno;}

private:
	bool loadBlock( const Index& elemno_);

private:
	DatabaseAdapter_BooleanBlock::Cursor m_dbadapter;
	BooleanBlock m_elemBlk;
	BooleanBlock::NodeCursor m_elemCursor;

	Index m_elemno;
};

}
#endif

