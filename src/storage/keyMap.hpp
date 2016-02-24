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
#ifndef _STRUS_LVDB_KEY_MAP_HPP_INCLUDED
#define _STRUS_LVDB_KEY_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "databaseAdapter.hpp"
#include "keyAllocatorInterface.hpp"
#include "private/utils.hpp"
#include "private/stringMap.hpp"
#include <cstdlib>
#include <string>
#include <cstring>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;
/// \brief Forward declaration
class KeyMapInv;

class KeyMap
{
public:
	KeyMap( DatabaseClientInterface* database_,
			DatabaseKey::KeyPrefix prefix_,
			DatabaseKey::KeyPrefix invprefix_,
			KeyAllocatorInterface* allocator_);
	KeyMap( DatabaseClientInterface* database_,
			DatabaseKey::KeyPrefix prefix_,
			KeyAllocatorInterface* allocator_);
	~KeyMap()
	{
		delete m_allocator;
	}

	void defineInv( KeyMapInv* invmap_);

	Index lookUp( const std::string& name);
	Index getOrCreate( const std::string& name, bool& isNew);

	void getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction);
	void getWriteBatch(
		DatabaseTransactionInterface* transaction);

	static bool isUnknown( const Index& value)
	{
		return value > UnknownValueHandleStart;
	}

	void deleteKey( const std::string& name);

private:
	void clear();
	void deleteAllFromDeletedList( DatabaseTransactionInterface* transaction);

private:
	enum {
		UnknownValueHandleStart=(1<<30)
	};

private:
	DatabaseClientInterface* m_database;
	DatabaseAdapter_StringIndex::ReadWriter m_dbadapter;
	DatabaseAdapter_IndexString::ReadWriter m_dbadapterinv;
	typedef StringMap<Index> Map;
	Map m_map;
	Index m_unknownHandleCount;
	KeyAllocatorInterface* m_allocator;
	KeyMapInv* m_invmap;
	StringVector m_deletedlist;
};

}//namespace
#endif


