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
#ifndef _STRUS_QUERY_KeyMap_HPP_INCLUDED
#define _STRUS_QUERY_KeyMap_HPP_INCLUDED
#include <string>
#include <map>
#include <boost/algorithm/string.hpp>

namespace strus {

struct KeyString
	:public std::string
{
	KeyString( const char* o)
		:std::string( boost::to_lower_copy( std::string(o))){}
	KeyString( const std::string& o)
		:std::string( boost::to_lower_copy( o)){}
	KeyString( const KeyString& o)
		:std::string( o){}
	KeyString(){}
};

/// \brief Case insensitive map of string to a value type defined a template argument
template <typename Value>
struct KeyMap
	:public std::map<KeyString,Value>
{
	typedef std::map<KeyString,Value> Parent;

	KeyMap(){}
	KeyMap( const KeyMap& o)
		:std::map<KeyString,Value>(o){}
	KeyMap( const std::map<std::string,Value>& o)
	{
		std::copy( o.begin(), o.end(), std::inserter( *this, this->end()));
	}

	void insert( const KeyString& key, const Value& value)
	{
		if (Parent::find( key) != this->end())
		{
			throw std::runtime_error( std::string( "duplicate definition of '") + key + "'");
		}
		Parent::operator[](key) = value;
	}

};

}//namespace
#endif

