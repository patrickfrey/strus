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
#ifndef _STRUS_QUERY_KEYMAP_HPP_INCLUDED
#define _STRUS_QUERY_KEYMAP_HPP_INCLUDED
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include "private/stringMap.hpp"
#include <string>
#include <map>

namespace strus {

/// \brief Case insensitive map of string to a value type defined a template argument
template <typename ValueType>
struct KeyMap
	:public StringMap<ValueType>
{
	typedef StringMap<ValueType> Parent;
	typedef typename Parent::const_iterator const_iterator;
	typedef typename Parent::iterator iterator;

	KeyMap(){}
	KeyMap( const KeyMap& o)
		:Parent(o){}

	const_iterator find( const char* key) const
	{
		return Parent::find(key);
	}

	iterator find( const char* key)
	{
		return Parent::find(key);
	}

	ValueType& operator[]( const char* key)
	{
		std::string keystr( utils::tolower( key));
		return Parent::operator[]( keystr);
	}

	ValueType& operator[]( const std::string& key)
	{
		std::string keystr( utils::tolower( key));
		return Parent::operator[]( keystr);
	}

	void insert( const std::string& key, const ValueType& value)
	{
		std::string keystr( utils::tolower( key));
		Parent::insert( key, value);
	}

	void insert( const char* key, const ValueType& value)
	{
		std::string keystr( utils::tolower( key));
		Parent::insert( key, value);
	}
};

}//namespace
#endif


