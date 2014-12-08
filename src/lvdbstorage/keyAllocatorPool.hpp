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
#ifndef _STRUS_LVDB_KEY_ALLOCATOR_POOL_HPP_INCLUDED
#define _STRUS_LVDB_KEY_ALLOCATOR_POOL_HPP_INCLUDED
#include "strus/index.hpp"
#include "keyAllocatorInterface.hpp"
#include "varSizeNodeTree.hpp"
#include <map>
#include <string>
#include <map>
#include <cstdlib>
#include <boost/thread/mutex.hpp>

namespace strus {

class KeyAllocatorPool
{
public:
	typedef unsigned int Handle;

public:
	KeyAllocatorPool( Index* globalCounter_, const Index& expectedSize)
		:m_occupied(0),m_globalCounter(globalCounter_){}
	~KeyAllocatorPool(){}

	Handle createHandle();
	void releaseHandle( Handle hnd);

	Index alloc( Handle hnd, const std::string& name, bool& isNew);

	Index allocRange( std::size_t size);

private:
	typedef VarSizeNodeTree KeyMap;

private:
	boost::mutex m_mutex;
	VarSizeNodeTree m_keymaps[64];
	uint64_t m_occupied;
	Index* m_globalCounter;
};

}//namespace
#endif

