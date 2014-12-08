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
#include "varSizeNodeTree.hpp"
#include <cstdlib>
#include <string>

namespace strus {

class KeyMap
{
public:
	KeyMap( leveldb::DB* db_, DatabaseKey::KeyPrefix prefix_,
			KeyAllocatorInterface* allocator_)
		:m_storage( db_, prefix_, false)
		,m_allocator(allocator_)
	{}
	~KeyMap()
	{
		delete m_allocator;
	}

	void done()
	{
		m_allocator->done();
	}

	Index lookUp( const std::string& name);
	Index getOrCreate( const std::string& name, bool& isnew);
	void store( const std::string& name, const Index& value);

	void getWriteBatch( leveldb::WriteBatch& batch);

	std::map<std::string,Index> getMap();
	std::map<Index,std::string> getInvMap();

private:
	Index allocValue( const std::string& name, bool& isNew)
	{
		return m_allocator->alloc( name, isNew);
	}

private:
	struct Value
	{
		Index counter;
		bool known;

		Value( Index counter_, bool known_)
			:counter(counter_),known(known_){}
		Value( const Value& o)
			:counter(o.counter),known(o.known){}
		Value()
			:counter(0),known(false){}
	};

private:
	KeyValueStorage m_storage;
	DatabaseKey::KeyPrefix m_prefix;
	VarSizeNodeTree m_map;
	KeyAllocatorInterface* m_allocator;
};

}//namespace
#endif


