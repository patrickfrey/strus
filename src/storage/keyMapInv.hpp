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
#ifndef _STRUS_STORAGE_KEY_MAP_INV_HPP_INCLUDED
#define _STRUS_STORAGE_KEY_MAP_INV_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/stringMap.hpp"
#include "private/utils.hpp"
#include <cstdlib>
#include <string>
#include <map>

namespace strus {

class KeyMapInv
{
public:
	KeyMapInv(){}
	KeyMapInv( const KeyMapInv& o)
		:m_map(o.m_map),m_strings(o.m_strings){}

	void set( const Index& idx, const std::string& value)
	{
		m_strings.push_back( value);
		m_map[ idx] = m_strings.back();
	}

	const char* get( const Index& idx) const
	{
		Map::const_iterator ei = m_map.find( idx);
		if (ei == m_map.end()) return 0;
		return ei->second;
	}
	void clear()
	{
		m_map.clear();
		m_strings.clear();
	}
	void erase( const Index& idx)
	{
		m_map.erase( idx);
	}

private:
	typedef utils::UnorderedMap<Index,const char*> Map;
	Map m_map;
	StringVector m_strings;
};

}//namespace
#endif


