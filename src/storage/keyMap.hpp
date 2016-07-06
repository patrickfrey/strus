/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_KEY_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_KEY_MAP_HPP_INCLUDED
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
	Index getOrCreate( const std::string& name);

	void getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction,
		int* nofNewItems=0);
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


