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
#include "databaseAdapter.hpp"
#include "keyMap.hpp"

using namespace strus;

void AttributeMap::renameNewDocNumbers( const std::map<Index,Index>& renamemap)
{
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index docno = BlockKey(mi->first).elem(1);
		if (KeyMap::isUnknown( docno))
		{
			Index varno = BlockKey(mi->first).elem(2);
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "attribute map");
			}
			BlockKey newkey( ri->second, varno);
			m_map[ newkey.index()] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void AttributeMap::defineAttribute( const Index& docno, const Index& varno, const std::string& value)
{
	BlockKey key( docno, varno);
	m_strings.push_back( value);
	m_map[ key.index()] = m_strings.back();
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

void AttributeMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	DeleteList::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		BlockKey key( *di);
		if (!key.elem(2))
		{
			DatabaseAdapter_DocAttribute::removeAll( transaction, key.elem(1));
		}
		else
		{
			DatabaseAdapter_DocAttribute::remove( transaction, key.elem(1), key.elem(2));
		}
	}

	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		BlockKey key( mi->first);
		DatabaseAdapter_DocAttribute::store(
				transaction, key.elem(1), key.elem(2),
				mi->second);
	}
	m_map.clear();
}


