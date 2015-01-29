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
#ifndef _STRUS_LVDB_ATTRIBUTE_MAP_HPP_INCLUDED
#define _STRUS_LVDB_ATTRIBUTE_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "blockKey.hpp"
#include "localStructAllocator.hpp"
#include <vector>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class AttributeMap
{
public:
	explicit AttributeMap( DatabaseInterface* database_)
		:m_database(database_)
	{
		m_strings.push_back( '\0');
	}

	~AttributeMap(){}

	void defineAttribute( const Index& docno, const Index& varno, const std::string& value);
	void deleteAttributes( const Index& docno);
	void deleteAttribute( const Index& docno, const Index& varno);

	void getWriteBatch( DatabaseTransactionInterface* transaction);

private:
	typedef LocalStructAllocator<std::pair<BlockKeyIndex,std::size_t> > MapAllocator;
	typedef std::less<BlockKeyIndex> MapCompare;
	typedef std::map<BlockKeyIndex,std::size_t,MapCompare,MapAllocator> Map;
	typedef std::vector<BlockKeyIndex> DeleteList;

private:
	DatabaseInterface* m_database;
	Map m_map;
	std::string m_strings;
	DeleteList m_deletes;
};

}//namespace
#endif

