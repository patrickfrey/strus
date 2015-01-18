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

Index KeyMap::lookUp( const std::string& name)
{
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (!value) return 0;
	char const* vi = value->ptr();
	char const* ve = vi + value->size();

	return unpackIndex( vi, ve);
}

Index KeyMap::getOrCreate( const std::string& name, bool& isNew)
{
	VarSizeNodeTree::NodeData data;
	if (m_globalmap && m_globalmap->find( name.c_str(), data))
	{
		isNew = false;
		return data;
	}
	if (m_map.find( name.c_str(), data))
	{
		isNew = false;
		return data;
	}
	const KeyValueStorage::Value* value = m_storage.load( name);
	if (value)
	{
		char const* vi = value->ptr();
		char const* ve = vi + value->size();

		Index rt = unpackIndex( vi, ve);
		store( name, rt);
		isNew = false;
		return rt;
	}
	else if (m_allocator->immediate())
	{
		return m_allocator->getOrCreate( name, isNew);
	}
	else
	{
		Index rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw std::runtime_error( "too many elements in keymap");
		}
		store( name, rt + UnknownValueHandleStart);
		return rt + UnknownValueHandleStart;
	}
}

void KeyMap::store( const std::string& name, const Index& value)
{
	m_map.set( name.c_str(), value);
}

void KeyMap::getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		leveldb::WriteBatch& batch)
{
	VarSizeNodeTree::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (mi.data() > UnknownValueHandleStart)
		{
			Index idx = lookUp( mi.key());
			if (!idx)
			{
				std::string valuestr;
				idx = m_allocator->alloc();
				packIndex( valuestr, idx);
				m_storage.store( mi.key(), valuestr, batch);
			}
			rewriteUnknownMap[ mi.data()] = idx;
		}
	}
	m_map.clear();
}


