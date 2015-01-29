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
#include "blockKey.hpp"
#include "indexPacker.hpp"
#include <cstdlib>
#include <string>
#include <map>
#include <cstring>
#include <stdexcept>

#error DEPRECATED

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;


/// \class KeyValueStorage
class KeyValueStorage
{
public:
	class Value
	{
	public:
		Value( const std::string& str)
			:m_ptr(str.c_str()),m_size(str.size()){}
		Value( const char* ptr_, std::size_t size_)
			:m_ptr(ptr_),m_size(size_){}
		Value( const char* ptr_)
			:m_ptr(ptr_),m_size(std::strlen(ptr_)){}
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
			DatabaseCursorInterface* dbcursor_,
			const DatabaseKey::KeyPrefix& keyprefix_)
		:m_dbcursor(dbcursor_)
		,m_keyprefix(keyprefix_)
		,m_curvalue()
	{}

	virtual ~KeyValueStorage()
	{}

	const Value* load( const std::string& key);
	const Value* load( const BlockKey& key, const Index& subnode=0);

	void store( const std::string& key, const Value& value, DatabaseTransactionInterface* transaction);
	void store( const BlockKey& key, const Index& subnode, const Value& value, DatabaseTransactionInterface* transaction);
	void store( const BlockKey& key, const Value& value, DatabaseTransactionInterface* transaction);

	void storeIm( DatabaseInterface* database, const std::string& key, const Value& value);
	void storeIm( DatabaseInterface* database, const BlockKey& key, const Index& subnode, const Value& value);
	void storeIm( DatabaseInterface* database, const BlockKey& key, const Value& value);

	void dispose( const std::string& key, DatabaseTransactionInterface* transaction);
	void dispose( const BlockKey& key, const Index& subnode, DatabaseTransactionInterface* transaction);
	void dispose( const BlockKey& key, DatabaseTransactionInterface* transaction);

	void disposeSubnodes( const BlockKey& key, DatabaseTransactionInterface* transaction);

	std::map<std::string,std::string> getMap();
	std::map<std::string,std::string> getInvMap();

private:
	DatabaseCursorInterface* m_dbcursor;
	DatabaseKey::KeyPrefix m_keyprefix;
	Value m_curvalue;
};

} //namespace
#endif

