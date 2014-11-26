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
#include "globalKeyMap.hpp"
#include "keyValueStorage.hpp"
#include "indexPacker.hpp"

using namespace strus;

Index GlobalKeyMap::lookUp( const std::string& name)
{
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (!value) return 0;
	char const* vi = value->ptr();
	char const* ve = vi + value->size();

	return unpackIndex( vi, ve);
}

Index GlobalKeyMap::getOrCreate( const std::string& name, Index& valuecnt)
{
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (value)
	{
		char const* vi = value->ptr();
		char const* ve = vi + value->size();

		return unpackIndex( vi, ve);
	}
	else
	{
		boost::mutex::scoped_lock( m_mutex);
		Map::const_iterator ki = m_map.find( name);
		if (ki != m_map.end())
		{
			return ki->second;
		}
		return m_map[ name] = valuecnt++;
	}
}

void GlobalKeyMap::store( const std::string& name, const Index& value)
{
	boost::mutex::scoped_lock( m_mutex);
	m_map[ name] = value;
}

void GlobalKeyMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		std::string valuestr;
		packIndex( valuestr, mi->second);
		m_storage.store( mi->first, valuestr, batch);
	}
	m_map.clear();
}

GlobalKeyMap::Map GlobalKeyMap::getMap()
{
	std::map<std::string,std::string> smap = m_storage.getMap();
	Map rt;
	std::map<std::string,std::string>::const_iterator si = smap.begin(), se = smap.end();
	for (; si != se; ++si)
	{
		char const* vi = si->second.c_str();
		const char* ve = vi + si->second.size();
		rt[ si->first] = unpackIndex( vi, ve);
	}
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		rt[ mi->first] = mi->second;
	}
	return rt;
}

GlobalKeyMap::InvMap GlobalKeyMap::getInvMap()
{
	std::map<std::string,std::string> smap = m_storage.getMap();
	InvMap rt;
	std::map<std::string,std::string>::const_iterator si = smap.begin(), se = smap.end();
	for (; si != se; ++si)
	{
		char const* vi = si->second.c_str();
		const char* ve = vi + si->second.size();
		rt[ unpackIndex( vi, ve)] = si->first;
	}
	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		rt[ mi->second] = mi->first;
	}
	return rt;
}


