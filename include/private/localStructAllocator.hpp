/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_LOCAL_STRUCT_ALLOCATOR_HPP_INCLUDED
#define _STRUS_LOCAL_STRUCT_ALLOCATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
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
			throw strus::runtime_error( _TXT( "illegal use of allocator, only one item can be allocated at once"));
		}
		StructType* rt = m_freelist.pop();
		if (!rt) return m_blkalloc.alloc();
		return rt;
	}

	void deallocate( pointer p, size_type n)
	{
		if (n != 1)
		{
			throw strus::runtime_error( _TXT( "illegal use of allocator, only on item can be deallocated at once"));
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
				std::size_t mm = m_allocsize?(m_allocsize*2):1024;
				if (mm < m_allocsize) throw std::bad_alloc();
				void* na = std::realloc( m_ar, mm * sizeof(StructType*));
				if (!na) throw std::bad_alloc();
				m_ar = (StructType**)na;
				m_allocsize = mm;
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
					std::size_t mm = m_allocsize?(m_allocsize*2):1024;
					if (mm * sizeof(Block*) < m_allocsize) throw std::bad_alloc();
					void* na = std::realloc( m_ar, mm * sizeof(Block*));
					if (!na) throw std::bad_alloc();
					m_ar = (Block**)na;
					m_allocsize = mm;
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


