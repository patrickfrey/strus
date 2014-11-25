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
#ifndef _STRUS_LVDB_KEY_VALUE_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_KEY_VALUE_STORAGE_HPP_INCLUDED
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <cstdlib>
#include <string>
#include <map>
#include <stdexcept>

namespace strus {

/// \class KeyValueStorage
class KeyValueStorage
{
public:
	class Key
	{
	public:
		Key()
		{}

		Key( const Index& id1_)
		{
			packIndex( m_keystr, id1_);
		}

		Key( const Index& id1_, const Index& id2_)
		{
			packIndex( m_keystr, id1_);
			packIndex( m_keystr, id2_);
		}

		Key( const std::string& o)
			:m_keystr( o){}

		Key( const Key& o)
			:m_keystr(o.m_keystr){}

		std::string str() const
		{
			return m_keystr;
		}

	private:
		friend class KeyValueStorage;
		std::string m_keystr;
	};

	class Value
	{
	public:
		Value( const std::string& str)
			:m_ptr(str.c_str()),m_size(str.size()){}
		Value( const char* ptr_, std::size_t size_)
			:m_ptr(ptr_),m_size(size_){}
		Value( const Value& o)
			:m_ptr(o.m_ptr),m_size(o.m_size){}
		Value()
			:m_ptr(0),m_size(0){}

		std::size_t size() const	{return m_size;}
		const char* ptr() const		{return m_ptr;}

		void init( const char* ptr_, std::size_t size_)
		{
			m_ptr = ptr_;
			m_size = size_;
		}

	private:
		const char* m_ptr;
		std::size_t m_size;
	};

public:
	KeyValueStorage(
			leveldb::DB* db_,
			const DatabaseKey::KeyPrefix& keyprefix_,
			bool useLruCache_)
		:m_db(db_)
		,m_keyprefix(keyprefix_)
		,m_curvaluestr()
	{
		m_readOptions.fill_cache = useLruCache_;
	}
	KeyValueStorage( const KeyValueStorage& o)
		:m_db(o.m_db)
		,m_readOptions(o.m_readOptions)
		,m_keyprefix(o.m_keyprefix)
		,m_curvaluestr(o.m_curvaluestr){}
	
	virtual ~KeyValueStorage()
	{}

	const Value* load( const Key& key);

	void store( const Key& key, const Value& value, leveldb::WriteBatch& batch);
	void dispose( const Key& key, leveldb::WriteBatch& batch);
	void disposeSubnodes( const Key& key, leveldb::WriteBatch& batch);

	std::map<std::string,std::string> getMap( const Key& key=Key());
	std::map<std::string,std::string> getInvMap( const Key& key=Key());

private:
	leveldb::DB* m_db;
	leveldb::ReadOptions m_readOptions;
	DatabaseKey::KeyPrefix m_keyprefix;
	std::string m_curvaluestr;
	Value m_curvalue;
};

} //namespace
#endif

