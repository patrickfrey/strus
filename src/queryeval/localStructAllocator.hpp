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
#ifndef _STRUS_LVDB_LOCAL_STRUCT_ALLOCATOR_HPP_INCLUDED
#define _STRUS_LVDB_LOCAL_STRUCT_ALLOCATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include <memory>
#include <stdexcept>
#include <cstdlib>

namespace strus
{

template <typename StructType>
class LocalStructAllocator
	:public std::allocator<StructType>
{
public:
	typedef size_t size_type;
	typedef StructType* pointer;
	typedef const StructType* const_pointer;

	template<typename _Tp1>
	struct rebind
	{
		typedef LocalStructAllocator<_Tp1> other;
	};

	pointer allocate( size_type n, const void *hint=0)
	{
		if (n != 1)
		{
			throw std::logic_error( "illegal use of allocator, only on item can be allocated at once");
		}
		StructType* rt = m_freelist.pop();
		if (!rt) return m_blkalloc.alloc();
		return rt;
	}

	void deallocate( pointer p, size_type n)
	{
		if (n != 1)
		{
			throw std::logic_error( "illegal use of allocator, only on item can be deallocated at once");
		}
		m_freelist.push( p);
	}

	LocalStructAllocator()
		:std::allocator<StructType>()
	{}

	LocalStructAllocator( const LocalStructAllocator &a)
		:std::allocator<StructType>(a)
	{}

	template <class U>
	LocalStructAllocator( const LocalStructAllocator<U> &a)
		:std::allocator<StructType>(a)
	{}

	~LocalStructAllocator() {}

private:
	enum {BlockSize=128};
	struct Block
	{
		StructType ar[ BlockSize];
	};
	struct FreeList
	{
		FreeList()
			:m_ar(0),m_size(0),m_allocsize(0){}
		~FreeList()
		{
			if (m_ar) std::free( m_ar);
		}

		void push( StructType* ptr)
		{
			if (m_size == m_allocsize)
			{
				void* na = std::realloc( m_ar, (m_allocsize + 1024) * sizeof(StructType*));
				if (!na) throw std::bad_alloc();
				m_ar = (StructType**)na;
				m_allocsize += 1024;
			}
			m_ar[ m_size++] = ptr;
		}
		StructType* pop()
		{
			if (!m_size) return 0;
			return m_ar[ --m_size];
		}

		StructType** m_ar;
		std::size_t m_size;
		std::size_t m_allocsize;
	};

	struct BlockAllocator
	{
		BlockAllocator()
			:m_ar(0),m_size(0),m_allocsize(0),m_blkidx(BlockSize){}
		~BlockAllocator()
		{
			std::size_t ii = 0;
			for (; ii<m_size; ++ii)
			{
				std::free( m_ar[ ii]);
			}
			std::free( m_ar);
		}

		StructType* alloc()
		{
			if (m_blkidx == BlockSize)
			{
				if (m_size == m_allocsize)
				{
					void* na = std::realloc( m_ar, (m_allocsize + 256) * sizeof(Block*));
					if (!na) throw std::bad_alloc();
					m_ar = (Block**)na;
					m_allocsize += 256;
				}
				void* mem = std::malloc( sizeof( Block));
				if (!mem) throw std::bad_alloc();
				m_ar[ m_size++] = (Block*)mem;
				m_blkidx = 0;
			}
			return &m_ar[ m_size-1]->ar[m_blkidx++];
		}
		Block** m_ar;
		std::size_t m_size;
		std::size_t m_allocsize;
		std::size_t m_blkidx;
	};

private:
	BlockAllocator m_blkalloc;
	FreeList m_freelist;
};
}//namespace
#endif


