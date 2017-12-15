/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERY_KEYMAP_HPP_INCLUDED
#define _STRUS_QUERY_KEYMAP_HPP_INCLUDED
#error WHO THE FUCK USES THIS
#include "private/internationalization.hpp"
#include "private/stringMap.hpp"
#include "strus/base/string_conv.hpp"
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
		std::string keystr( string_conv::tolower( key));
		return Parent::operator[]( keystr);
	}

	ValueType& operator[]( const std::string& key)
	{
		std::string keystr( string_conv::tolower( key));
		return Parent::operator[]( keystr);
	}

	void insert( const std::string& key, const ValueType& value)
	{
		std::string keystr( string_conv::tolower( key));
		Parent::insert( key, value);
	}

	void insert( const char* key, const ValueType& value)
	{
		std::string keystr( string_conv::tolower( key));
		Parent::insert( key, value);
	}
};

}//namespace
#endif


