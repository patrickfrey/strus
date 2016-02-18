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
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"

using namespace strus;

KeyMap::KeyMap( DatabaseClientInterface* database_,
		DatabaseKey::KeyPrefix prefix_,
		KeyAllocatorInterface* allocator_)
	:m_dbadapter(prefix_,database_)
	,m_unknownHandleCount(0)
	,m_allocator(allocator_)
	,m_invmap(0)
{}

Index KeyMap::lookUp( const std::string& name)
{
	return m_dbadapter.get( name);
}

Index KeyMap::getOrCreate( const std::string& name, bool& isNew)
{
	Map::const_iterator mi = m_map.find( name);
	if (mi != m_map.end())
	{
		isNew = false;
		return mi->second;
	}
	Index rt;
	if (m_dbadapter.load( name,rt))
	{
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		isNew = false;
	}
	else if (m_allocator->immediate())
	{
		rt = m_allocator->getOrCreate( name, isNew);
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		isNew = true;
	}
	else
	{
		rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw strus::runtime_error( _TXT( "too many elements in keymap"));
		}
		rt += UnknownValueHandleStart;
		m_map[ name] = rt;
		if (m_invmap) m_invmap->set( rt, name);
		isNew = true;
	}
	return rt;
}

void KeyMap::getWriteBatch(
	DatabaseTransactionInterface* transaction)
{
	StringVector::const_iterator di = m_deletedlist.begin(), de = m_deletedlist.end();
	for (; di != de; ++di)
	{
		m_dbadapter.remove( transaction, *di);
	}
	// Clear maps:
	clear();
}

void KeyMap::getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction)
{
	StringVector::const_iterator di = m_deletedlist.begin(), de = m_deletedlist.end();
	for (; di != de; ++di)
	{
		m_dbadapter.remove( transaction, *di);
	}
	Map::iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (mi->second > UnknownValueHandleStart)
		{
			Index idx = lookUp( mi->first);
			if (!idx)
			{
				idx = m_allocator->alloc();
				m_dbadapter.store( transaction, mi->first, idx);
			}
			rewriteUnknownMap[ mi->second] = idx;
			if (m_invmap) m_invmap->set( idx, mi->first);
			mi->second = idx;
		}
	}
	// Clear maps:
	clear();
}

void KeyMap::deleteKey( const std::string& name)
{
	if (m_invmap)
	{
		Map::iterator mi = m_map.find( name);
		if (mi != m_map.end())
		{
			m_invmap->erase( mi->second);
		}
		m_map.erase( mi);
	}
	else
	{
		m_map.erase( name);
	}
	m_deletedlist.push_back( name);
}

void KeyMap::clear()
{
	m_map.clear();
	m_unknownHandleCount = 0;
	if (m_invmap) m_invmap->clear();
	m_deletedlist.clear();
}


