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
#ifndef _STRUS_LVDB_KEY_MAP_INV_HPP_INCLUDED
#define _STRUS_LVDB_KEY_MAP_INV_HPP_INCLUDED
#include "strus/index.hpp"
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
		m_map[ idx] = m_strings.size();
		m_strings.append( value);
		m_strings.push_back( '\0');
	}

	const char* get( const Index& idx) const
	{
		std::map<Index,std::size_t>::const_iterator ei = m_map.find( idx);
		if (ei == m_map.end()) return 0;
		return m_strings.c_str() + ei->second;
	}

private:
	std::map<Index,std::size_t> m_map;
	std::string m_strings;
};

}//namespace
#endif

