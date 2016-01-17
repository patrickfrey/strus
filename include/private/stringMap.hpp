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
#ifndef _STRUS_STRING_MAP_HPP_INCLUDED
#define _STRUS_STRING_MAP_HPP_INCLUDED
#include "private/internationalization.hpp"
#include "private/localStructAllocator.hpp"
#include <map>
#include <list>
#include <vector>
#include <string>
#include <cstring>

namespace strus
{

class StringMapKeyBlock
{
public:
	enum {DefaultSize = 16300};

public:
	explicit StringMapKeyBlock( std::size_t blksize_=DefaultSize);
	StringMapKeyBlock( const StringMapKeyBlock& o);
	~StringMapKeyBlock();

	const char* allocKey( const std::string& key);
	const char* allocKey( const char* key, std::size_t keylen);

private:
	char* m_blk;
	std::size_t m_blksize;
	std::size_t m_blkpos;
};

class StringMapKeyBlockList
{
public:
	StringMapKeyBlockList(){}
	StringMapKeyBlockList( const StringMapKeyBlockList& o)
		:m_ar(o.m_ar){}

	const char* allocKey( const char* key, std::size_t keylen);
	void clear();

private:
	std::list<StringMapKeyBlock> m_ar;
};


class StringVector
{
public:
	void push_back( const char* value)
	{
		const char* valuedup = m_blkar.allocKey( value, std::strlen( value));
		return m_ar.push_back( valuedup);
	}
	void push_back( const char* value, std::size_t valuesize)
	{
		const char* valuedup = m_blkar.allocKey( value, valuesize);
		return m_ar.push_back( valuedup);
	}
	void push_back( const std::string& value)
	{
		const char* valuedup = m_blkar.allocKey( value.c_str(), value.size());
		return m_ar.push_back( valuedup);
	}

	typedef std::vector<const char*>::const_iterator const_iterator;
	typedef std::vector<const char*>::iterator iterator;

	std::vector<const char*>::const_iterator begin() const		{return m_ar.begin();}
	std::vector<const char*>::const_iterator end() const		{return m_ar.end();}

	const char* operator[]( std::size_t i) const			{return m_ar[i];}
	const char* back() const					{return m_ar.back();}
	std::size_t size() const					{return m_ar.size();}

	void clear()
	{
		m_ar.clear();
		m_blkar.clear();
	}
private:
	std::vector<const char*> m_ar;
	StringMapKeyBlockList m_blkar;
};


template <typename ValueType>
class StringMap
{
private:
	struct MapKeyCompare
	{
		bool operator()( char const *a, char const *b) const
		{
			return std::strcmp( a, b) < 0;
		}
	};

	typedef LocalStructAllocator<std::pair<const char*,ValueType> > MapAllocator;
	typedef std::map<const char*,ValueType, MapKeyCompare, MapAllocator> Map;

public:
	StringMap(){}
	StringMap( const StringMap& o)
		:m_map(o.m_map),m_keystring_blocks(o.m_keystring_blocks){}

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
		const char* keydup = m_keystring_blocks.allocKey( key, std::strlen( key));
		return m_map[ keydup];
	}

	ValueType& operator[]( const std::string& key)
	{
		iterator itr = find( key.c_str());
		if (itr != end())
		{
			return itr->second;
		}
		const char* keydup = m_keystring_blocks.allocKey( key.c_str(), key.size());
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

	void clear()
	{
		m_map.clear();
		m_keystring_blocks.clear();
	}

private:
	Map m_map;
	StringMapKeyBlockList m_keystring_blocks;
};

}//namespace
#endif


