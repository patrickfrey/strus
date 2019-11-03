/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STRING_MAP_HPP_INCLUDED
#define _STRUS_STRING_MAP_HPP_INCLUDED
#include "private/internationalization.hpp"
#include "strus/base/crc32.hpp"
#include "strus/base/unordered_map.hpp"
#include "strus/base/symbolTable.hpp"
#include <map>
#include <list>
#include <vector>
#include <string>
#include <cstring>

namespace strus
{

template <typename ValueType>
class StringMap
{
private:
	struct MapKeyLess
	{
		bool operator()( char const *a, char const *b) const
		{
			return std::strcmp( a, b) < 0;
		}
	};
	struct MapKeyEqual
	{
		bool operator()( char const *a, char const *b) const
		{
			return std::strcmp( a, b) == 0;
		}
	};
	struct HashFunc{
		int operator()( const char * str)const
		{
			return utils::Crc32::calc( str);
		}
	};

	typedef strus::unordered_map<const char*,ValueType,HashFunc,MapKeyEqual> Map;

public:
	StringMap(){}

	typedef typename Map::const_iterator const_iterator;
	typedef typename Map::iterator iterator;

	const_iterator begin() const				{return m_map.begin();}
	const_iterator end() const				{return m_map.end();}

	iterator begin()					{return m_map.begin();}
	iterator end()						{return m_map.end();}

	const_iterator find( const char* key) const		{return m_map.find( key);}
	iterator find( const char* key)				{return m_map.find( key);}

	const_iterator find( const std::string& key) const	{return m_map.find( key.c_str());}
	iterator find( const std::string& key)			{return m_map.find( key.c_str());}

	ValueType& operator[]( const char* key)
	{
		iterator itr = find( key);
		if (itr != end())
		{
			return itr->second;
		}
		const char* keydup = m_keystring_blocks.allocStringCopy( key, std::strlen( key));
		return m_map[ keydup];
	}

	ValueType& operator[]( const std::string& key)
	{
		iterator itr = find( key.c_str());
		if (itr != end())
		{
			return itr->second;
		}
		const char* keydup = m_keystring_blocks.allocStringCopy( key.c_str(), key.size());
		return m_map[ keydup];
	}

	void insert( const std::string& key, const ValueType& value)
	{
		operator[]( key) = value;
	}

	void insert( const char* key, const ValueType& value)
	{
		operator[]( key) = value;
	}

	void erase( const char* key)
	{
		m_map.erase( key);
	}

	void erase( const std::string& key)
	{
		m_map.erase( key.c_str());
	}

	void erase( iterator keyitr)
	{
		m_map.erase( keyitr);
	}

	void clear()
	{
		m_map.clear();
		m_keystring_blocks.clear();
	}

private:
#if __cplusplus >= 201103L
	StringMap( StringMap&) = delete;	//... non copyable
	void operator=( StringMap&) = delete;	//... non copyable
#endif
private:
	Map m_map;
	BlockAllocator m_keystring_blocks;
};

}//namespace
#endif


