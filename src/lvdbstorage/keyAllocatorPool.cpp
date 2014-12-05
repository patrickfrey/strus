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
#include "keyAllocatorPool.hpp"
#include "bitOperations.hpp"
#include <limits>

using namespace strus;

KeyAllocatorPool::Handle KeyAllocatorPool::createHandle()
{
	boost::mutex::scoped_lock( m_mutex);
	Handle rt = BitOperations::bitScanForward( 0xffffFFFFffffFFFFUL ^ m_occupied);
	if (!rt)
	{
		throw std::runtime_error( "no key allocator pool handles left (number of simultaneus transactions bigger than 64)");
	}
	m_occupied += (1 << (rt-1));
	return rt;
}

void KeyAllocatorPool::releaseHandle( Handle hnd)
{
	boost::mutex::scoped_lock( m_mutex);
	m_keymaps[ hnd-1].clear();
	if (!(m_occupied & (1 << (hnd-1))))
	{
		throw std::logic_error("invalid release key alloc pool handle");
	}
	m_occupied -= (1 << (hnd-1));
}

static inline unsigned int nextHandle( uint64_t& itr)
{
	unsigned int rt = BitOperations::bitScanForward( itr);
	if (rt)
	{
		itr ^= (1 << (rt-1));
	}
	return rt;
}

Index KeyAllocatorPool::alloc( Handle hnd, const std::string& name, bool& isNew)
{
	if (!hnd) throw std::runtime_error( "using invalid key alloc pool handle (NULL)");

	boost::mutex::scoped_lock( m_mutex);
	uint64_t itr = m_occupied;
	std::size_t hh = nextHandle( itr);
	for (; hh; hh=nextHandle( itr))
	{
		KeyMap::const_iterator ki = m_keymaps[ hh-1].find( name);
		if (ki != m_keymaps[ hh-1].end())
		{
			if (hh == hnd)
			{
				throw std::logic_error( "inconsistency in key allocator pool maps. Handle found where not expected");
			}
			return ki->second;
		}
	}
	Index rt = *m_globalCounter;
	*m_globalCounter += 1;
	if (rt >= std::numeric_limits<Index>::max() - 1)
	{
		throw std::runtime_error( "alloc range of index failed (too many items allocated)");
	}
	m_keymaps[ hnd-1][ name] = rt;
	return rt;
}

Index KeyAllocatorPool::allocRange( std::size_t size)
{
	boost::mutex::scoped_lock( m_mutex);
	if (*m_globalCounter >= (Index)(std::numeric_limits<Index>::max() - size))
	{
		throw std::runtime_error( "alloc range of index failed (too many items allocated)");
	}
	Index rt = *m_globalCounter;
	*m_globalCounter += size;
	return rt;
}


