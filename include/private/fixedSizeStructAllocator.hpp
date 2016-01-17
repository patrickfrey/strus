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
#ifndef _STRUS_FIXED_STRUCT_ALLOCATOR_HPP_INCLUDED
#define _STRUS_FIXED_STRUCT_ALLOCATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <memory>
#include <stdexcept>
#include <cstdlib>

namespace strus
{

template <typename StructType, unsigned int MAXSIZE>
class FixedSizeStructAllocator
	:public std::allocator<StructType>
{
public:
	typedef size_t size_type;
	typedef StructType* pointer;
	typedef const StructType* const_pointer;

	template<typename _Tp1>
	struct rebind
	{
		typedef FixedSizeStructAllocator<_Tp1, MAXSIZE> other;
	};

	pointer allocate( size_type n, const void *hint=0)
	{
		if (n != 1)
		{
			throw strus::logic_error( _TXT( "illegal use of allocator, only one item can be allocated at once"));
		}
		StructType* rt = m_freelist.pop();
		if (!rt) return alloc();
		return rt;
	}

	void deallocate( pointer p, size_type n)
	{
		if (n != 1)
		{
			throw strus::logic_error( _TXT( "illegal use of allocator, only on item can be deallocated at once"));
		}
		m_freelist.push( p);
	}

	FixedSizeStructAllocator()
		:std::allocator<StructType>(),m_aridx(0)
	{
		m_freelist.init( m_ar, 0);
	}

	FixedSizeStructAllocator( const FixedSizeStructAllocator& o)
		:std::allocator<StructType>(o),m_aridx(o.m_aridx)
	{
		std::memcpy( m_ar, o.m_ar, sizeof( m_ar));
		m_freelist.init( m_ar, o.m_freelist.m_idx);
	}

	template <class U>
	FixedSizeStructAllocator( const FixedSizeStructAllocator<U,MAXSIZE> &o)
		:std::allocator<StructType>(o),m_aridx(o.m_aridx)
	{
		unsigned int elemsize = (sizeof(U) < sizeof(StructType))?sizeof(U):sizeof(StructType);
		unsigned int oi = 0, oe = o.m_freelist.m_idx==0?o.m_aridx:MAXSIZE;
		for (; oi != oe; ++oi)
		{
			std::memcpy( m_ar+oi, o.m_ar+oi, elemsize);
		}
		m_freelist.init( m_ar, o.m_freelist.m_idx);
	}

	~FixedSizeStructAllocator() {}

	bool empty() const
	{
		return (m_aridx == 0);
	}

private:
	struct FreeList
	{
		FreeList()
			:m_ar(0),m_idx(0){}
		~FreeList()
		{}

		void init( StructType* ar_, unsigned int idx_)
		{
			m_ar = ar_;
			m_idx = idx_;
		}

		void push( StructType* ptr)
		{
			unsigned int idx = (ptr-m_ar);
			*(unsigned int*)(void*)(m_ar+idx) = m_idx;
			m_idx = idx+1;
		}
		StructType* pop()
		{
			if (!m_idx) return 0;
			StructType* rt = m_ar + m_idx - 1;
			m_idx = *(unsigned int*)(void*)(rt);
			return rt;
		}

		StructType* m_ar;
		unsigned int m_idx;
	};

	StructType* alloc()
	{
		if (m_aridx >= MAXSIZE) throw std::bad_alloc();
		return &m_ar[ m_aridx++];
	}

private:
	template <class U, unsigned int M> friend class FixedSizeStructAllocator;
	StructType m_ar[ MAXSIZE];
	unsigned int m_aridx;
	FreeList m_freelist;
};
}//namespace
#endif


