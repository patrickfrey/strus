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
#ifndef _STRUS_LVDB_DATA_BLOCK_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_DATA_BLOCK_STORAGE_HPP_INCLUDED
#include "dataBlock.hpp"
#include "databaseKey.hpp"
#include "strus/databaseCursorInterface.hpp"
#include "strus/reference.hpp"
#include <cstdlib>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

/// \class DataBlockStorage
class DataBlockStorage
{
public:
	DataBlockStorage( DatabaseInterface* database_, const DatabaseKey& key_, bool useLruCache_);

	virtual ~DataBlockStorage();

	const DataBlock* curblock() const
	{
		return &m_curblock;
	}

	const DataBlock* load( const Index& id);
	const DataBlock* loadLast();
	const DataBlock* loadFirst();
	const DataBlock* loadNext();

	void store( const DataBlock& block, DatabaseTransactionInterface* transaction);
	void dispose( const Index& id, DatabaseTransactionInterface* transaction);

private:
	const DataBlock*
		extractData(
			const DatabaseCursorInterface::Slice& key,
			const DatabaseCursorInterface::Slice& value);

private:
	DatabaseInterface* m_database;
	Reference<DatabaseCursorInterface> m_cursor;
	DatabaseKey m_key;
	std::size_t m_keysize;
	DataBlock m_curblock;
};

} //namespace
#endif

