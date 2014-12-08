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
#include "keyValueStorage.hpp"
#include "indexPacker.hpp"

using namespace strus;

enum {FlagUnknown=0, FlagKnown=(1U<<31), MaskKnown=(1U<<31)-1};

Index KeyMap::lookUp( const std::string& name)
{
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (!value) return 0;
	char const* vi = value->ptr();
	char const* ve = vi + value->size();

	return unpackIndex( vi, ve);
}

Index KeyMap::getOrCreate( const std::string& name, bool& isnew)
{
	VarSizeNodeTree::NodeData data;
	if (m_map.find( name.c_str(), data))
	{
		isnew = false;
		return data & MaskKnown;
	}
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (value)
	{
		char const* vi = value->ptr();
		char const* ve = vi + value->size();

		isnew = false;
		Index rt = unpackIndex( vi, ve);
		m_map.set( name.c_str(), rt | FlagKnown);
		return rt;
	}
	else
	{
		isnew = true;
		Index rt = allocValue( name, isnew);
		m_map.set( name.c_str(), rt | FlagUnknown);
		return rt;
	}
}

void KeyMap::store( const std::string& name, const Index& value)
{
	m_map.set( name.c_str(), value | FlagUnknown);
}

void KeyMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	VarSizeNodeTree::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (0 == (mi.data() & FlagKnown))
		{
			std::string valuestr;
			packIndex( valuestr, mi.data() & MaskKnown);
			m_storage.store( mi.key(), valuestr, batch);
		}
	}
	m_map.clear();
}

std::map<std::string,Index> KeyMap::getMap()
{
	std::map<std::string,std::string> smap = m_storage.getMap();
	std::map<std::string,Index> rt;
	std::map<std::string,std::string>::const_iterator si = smap.begin(), se = smap.end();
	for (; si != se; ++si)
	{
		char const* vi = si->second.c_str();
		const char* ve = vi + si->second.size();
		rt[ si->first] = unpackIndex( vi, ve);
	}
	VarSizeNodeTree::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		rt[ mi.key()] = mi.data() & MaskKnown;
	}
	return rt;
}

std::map<Index,std::string> KeyMap::getInvMap()
{
	std::map<std::string,std::string> smap = m_storage.getMap();
	std::map<Index,std::string> rt;
	std::map<std::string,std::string>::const_iterator si = smap.begin(), se = smap.end();
	for (; si != se; ++si)
	{
		char const* vi = si->second.c_str();
		const char* ve = vi + si->second.size();
		rt[ unpackIndex( vi, ve)] = si->first;
	}
	VarSizeNodeTree::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		rt[ mi.data() & MaskKnown] = mi.key();
	}
	return rt;
}


