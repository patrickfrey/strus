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
#include "attributeMap.hpp"
#include "databaseKey.hpp"
#include "keyValueStorage.hpp"

using namespace strus;

void AttributeMap::defineAttribute( const Index& docno, const Index& varno, const std::string& value)
{
	BlockKey key( docno, varno);
	m_map[ key.index()] = m_strings.size();
	m_strings.append( value);
	m_strings.push_back( '\0');
}

void AttributeMap::deleteAttributes( const Index& docno)
{
	BlockKey key( docno, 1);
	Map::iterator mi = m_map.upper_bound( key.index()-1), me = m_map.end();
	Map::iterator rangeStart = mi;
	for (;mi != me; ++mi)
	{
		BlockKey iterkey( mi->first);
		if (iterkey.elem( 1) != docno)
		{
			break;
		}
	}
	if (rangeStart != me)
	{
		m_map.erase( rangeStart, mi);
	}
	m_deletes.push_back( BlockKey(docno).index());
}

void AttributeMap::deleteAttribute( const Index& docno, const Index& varno)
{
	BlockKey key( docno, varno);
	Map::iterator mi = m_map.find( key.index());
	if (mi != m_map.end())
	{
		m_map.erase( mi);
	}
	m_deletes.push_back( key.index());
}

void AttributeMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	KeyValueStorage attrstorage( m_db, DatabaseKey::DocAttributePrefix, false);

	DeleteList::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		BlockKey key( *di);
		if (!key.elem(2))
		{
			attrstorage.disposeSubnodes( key, batch);
		}
		else
		{
			attrstorage.dispose( key, batch);
		}
	}

	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		BlockKey key( mi->first);
		attrstorage.store( key, KeyValueStorage::Value( m_strings.c_str() + mi->second), batch);
	}
	m_map.clear();
}


