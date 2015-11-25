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
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"

using namespace strus;

KeyMap::KeyMap( DatabaseClientInterface* database_,
		DatabaseKey::KeyPrefix prefix_,
		KeyAllocatorInterface* allocator_,
		const conotrie::CompactNodeTrie* globalmap_)
	:m_dbadapter(prefix_,database_)
	,m_maxCachedKeyLen(DefaultMaxCachedKeyLen)
	,m_globalmap(globalmap_)
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
	conotrie::CompactNodeTrie::NodeData data;
	if (m_globalmap && m_globalmap->get( name.c_str(), data))
	{
		isNew = false;
		if (m_invmap)
		{
			if (!m_invmap->get( data)) m_invmap->set( data, name);
		}
		return data;
	}
	if (m_map.get( name.c_str(), data))
	{
		isNew = false;
		return data;
	}
	Index rt;
	if (m_dbadapter.load( name,rt))
	{
		(void)m_map.set( name.c_str(), rt);
		if (m_invmap) m_invmap->set( data, name);
		isNew = false;
	}
	else if (m_allocator->immediate())
	{
		rt = m_allocator->getOrCreate( name, isNew);
		if (m_invmap) m_invmap->set( data, name);
	}
	else
	{
		OverflowMap::const_iterator oi = m_overflowmap.find( name);
		if (oi != m_overflowmap.end())
		{
			return oi->second;
		}
		rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw strus::runtime_error( _TXT( "too many elements in keymap"));
		}
		rt += UnknownValueHandleStart;
		if (name.size() > m_maxCachedKeyLen || !m_map.set( name.c_str(), rt))
		{
			// ... Too many elements in the map or the key is bigger than the limit
			//	defined as reasonable for a compact node trie. We are switching
			//	to an STL map for caching the value:
			m_overflowmap[ name] = rt;
		}
	}
	return rt;
}

void KeyMap::getWriteBatch(
	DatabaseTransactionInterface* transaction)
{
	std::map<Index,Index> unknownMap;
	getWriteBatch( unknownMap, transaction);
	if (!unknownMap.empty())
	{
		throw strus::runtime_error( _TXT( "internal: unexpected unknown elements in keymap"));
	}
}

void KeyMap::getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction)
{
	conotrie::CompactNodeTrie::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (mi.data() > UnknownValueHandleStart)
		{
			Index idx = lookUp( mi.key());
			if (!idx)
			{
				idx = m_allocator->alloc();
				m_dbadapter.store( transaction, mi.key(), idx);
			}
			rewriteUnknownMap[ mi.data()] = idx;
			if (m_invmap) m_invmap->set( idx, mi.key());
		}
	}
	OverflowMap::const_iterator
		oi = m_overflowmap.begin(), oe = m_overflowmap.end();
	for (; oi != oe; ++oi)
	{
		if (oi->second > UnknownValueHandleStart)
		{
			Index idx = lookUp( oi->first);
			if (!idx)
			{
				idx = m_allocator->alloc();
				m_dbadapter.store( transaction, oi->first, idx);
			}
			rewriteUnknownMap[ oi->second] = idx;
			if (m_invmap) m_invmap->set( idx, oi->first);
		}
	}
	StringVector::const_iterator di = m_deletedlist.begin(), de = m_deletedlist.end();
	for (; di != de; ++di)
	{
		m_dbadapter.remove( transaction, *di);
	}

	m_map.clear();
	m_overflowmap.clear();
}


