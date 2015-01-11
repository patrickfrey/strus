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
#include "keyValueStorage.hpp"
#include "keyAllocatorInterface.hpp"
#include "keyStorageInterface.hpp"
#include "varSizeNodeTree.hpp"
#include <cstdlib>
#include <string>

namespace strus {

class KeyMap
{
public:
	KeyMap( leveldb::DB* db_,
			DatabaseKey::KeyPrefix prefix_,
			KeyAllocatorInterface* allocator_,
			const VarSizeNodeTree* globalmap_=0)
		:m_storage( db_, prefix_, false)
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
	void store( const std::string& name, const Index& value);

	void getWriteBatch(
		std::map<Index,Index>& rewriteUnknownMap,
		leveldb::WriteBatch& batch);

	static bool isUnknown( const Index& value)
	{
		return value > UnknownValueHandleStart;
	}

	std::map<std::string,Index> getMap();
	std::map<Index,std::string> getInvMap();

private:
	enum {
		UnknownValueHandleStart=(1<<30)
	};

private:
	KeyValueStorage m_storage;
	VarSizeNodeTree m_map;
	const VarSizeNodeTree* m_globalmap;
	Index m_unknownHandleCount;
	KeyAllocatorInterface* m_allocator;
};

}//namespace
#endif


