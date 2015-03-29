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
		rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw strus::runtime_error( _TXT( "too many elements in keymap"));
		}
		rt += UnknownValueHandleStart;
		if (!m_map.set( name.c_str(), rt))
		{
			// ... Too many elements in the map, we have to switch to an STL map
			m_overflow_map[ name] = rt;
		}
	}
	return rt;
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
	m_map.clear();

	std::map<std::string,Index>::const_iterator
		oi = m_overflow_map.begin(), oe = m_overflow_map.end();
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
}


