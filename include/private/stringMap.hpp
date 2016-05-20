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
#include <boost/unordered_map.hpp>
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

	typedef boost::unordered_map<const char*,ValueType,HashFunc,MapKeyEqual> Map;

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
	Map m_map;
	StringMapKeyBlockList m_keystring_blocks;
};

}//namespace
#endif


