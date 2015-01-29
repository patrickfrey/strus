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
#include "databaseRecord.hpp"

using namespace strus;

Index KeyMap::lookUp( const std::string& name)
{
	return DatabaseRecord_StringIndex_Base::get( (char)m_prefix, m_database, name);
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
	Index rt;
	if (DatabaseRecord_StringIndex_Base::load( (char)m_prefix, m_database, name, rt))
	{
		m_map.set( name.c_str(), rt);
		isNew = false;
	}
	else if (m_allocator->immediate())
	{
		rt = m_allocator->getOrCreate( name, isNew);
	}
	else
	{
		rt = ++m_unknownHandleCount;
		if (rt >= UnknownValueHandleStart)
		{
			throw std::runtime_error( "too many elements in keymap");
		}
		rt += UnknownValueHandleStart;
		m_map.set( name.c_str(), rt);
	}
	return rt;
}

void KeyMap::getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction)
{
	VarSizeNodeTree::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		if (mi.data() > UnknownValueHandleStart)
		{
			Index idx = lookUp( mi.key());
			if (!idx)
			{
				idx = m_allocator->alloc();
				DatabaseRecord_StringIndex_Base::store(
					(char)m_prefix, transaction, mi.key(), idx);
			}
			rewriteUnknownMap[ mi.data()] = idx;
		}
	}
	m_map.clear();
}


