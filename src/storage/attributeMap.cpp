/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "attributeMap.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"
#include <algorithm>

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
	DeleteList newdeletes;
	DeleteList::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		Index docno = BlockKey(*di).elem(1);
		if (KeyMap::isUnknown( docno))
		{
			Index varno = BlockKey(*di).elem(2);
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "attribute map");
			}
			if (varno)
			{
				newdeletes.push_back( BlockKey( ri->second, varno).index());
			}
			else
			{
				newdeletes.push_back( BlockKey( ri->second).index());
			}
		}
		else
		{
			newdeletes.push_back( *di);
		}
	}
	m_deletes = newdeletes;
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
}

void AttributeMap::clear()
{
	m_map.clear();
	m_strings.clear();
	m_deletes.clear();
}

void AttributeMap::swap( AttributeMap& o)
{
	std::swap( m_database, o.m_database);
	m_map.swap( o.m_map);
	m_strings.swap( o.m_strings);
	m_deletes.swap( o.m_deletes);
}

