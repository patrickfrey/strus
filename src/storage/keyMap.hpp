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
#ifndef _STRUS_LVDB_KEY_MAP_HPP_INCLUDED
#define _STRUS_LVDB_KEY_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "databaseKey.hpp"
#include "keyAllocatorInterface.hpp"
#include "varSizeNodeTree.hpp"
#include <cstdlib>
#include <string>
#include <map>

namespace strus {

/// \brief Forward declaration
class DatabaseInterface;
/// \brief Forward declaration
class DatabaseCursorInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class KeyMap
{
public:
	KeyMap( DatabaseInterface* database_,
			DatabaseKey::KeyPrefix prefix_,
			KeyAllocatorInterface* allocator_,
			const VarSizeNodeTree* globalmap_=0)
		:m_database(database_)
		,m_prefix(prefix_)
		,m_globalmap(globalmap_)
		,m_unknownHandleCount(0)
		,m_allocator(allocator_)
	{}
	~KeyMap()
	{
		delete m_allocator;
	}

	Index lookUp( const std::string& name);
	Index getOrCreate( const std::string& name, bool& isNew);

	void getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		DatabaseTransactionInterface* transaction);

	static bool isUnknown( const Index& value)
	{
		return value > UnknownValueHandleStart;
	}

private:
	enum {
		UnknownValueHandleStart=(1<<30)
	};

private:
	DatabaseInterface* m_database;
	DatabaseKey::KeyPrefix m_prefix;
	VarSizeNodeTree m_map;
	const VarSizeNodeTree* m_globalmap;
	Index m_unknownHandleCount;
	KeyAllocatorInterface* m_allocator;
};

}//namespace
#endif


