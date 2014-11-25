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
#ifndef _STRUS_REFERENCE_HPP_INCLUDED
#define _STRUS_REFERENCE_HPP_INCLUDED
#include "strus/summarizerInterface.hpp"
#include <stdexcept>
#include <cstdlib>

namespace strus
{

/// \brief Local reference
template <class TYPE>
class LocalReference
{
public:
	LocalReference()
		:m_ref(0){}
	LocalReference( TYPE* o)
		:m_ref(o){}

	virtual ~LocalReference()
	{
		if (m_ref) delete m_ref;
	}

	void reset( TYPE* ref_)
	{
		if (m_ref) delete m_ref;
		m_ref = ref_;
	}

	TYPE* get() const
	{
		return m_ref;
	}

	TYPE* detach()
	{
		TYPE* rt = m_ref;
		m_ref = 0;
		return rt;
	}

	TYPE* operator->() const
	{
		return m_ref;
	}

	TYPE& operator*() const
	{
		return *m_ref;
	}

private:
	TYPE* m_ref;
};


/// \brief Local reference
template <class TYPE>
class LocalReferenceArray
{
public:
	LocalReferenceArray()
		:m_ar(0),m_allocsize(0),m_size(0){}

	virtual ~LocalReferenceArray()
	{
		clear();
		delete m_ar;
	}

	const TYPE& operator[]( std::size_t idx) const
	{
		if (idx>=m_size) throw std::logic_error( "array bound read (LocalReferecnceArray)");
		return *m_ar[ idx];
	}
	TYPE& operator[]( std::size_t idx)
	{
		if (idx>=m_size) throw std::logic_error( "array bound read (LocalReferecnceArray)");
		return *m_ar[ idx];
	}

	const TYPE& back() const
	{
		if (0==m_size) throw std::logic_error( "array bound read (LocalReferecnceArray)");
		return *m_ar[ m_size-1];
	}
	TYPE& back()
	{
		if (0==m_size) throw std::logic_error( "array bound read (LocalReferecnceArray)");
		return *m_ar[ m_size-1];
	}

	enum {MemIncrement=32};
	void push_back( TYPE* obj)
	{
		if (m_size == m_allocsize)
		{
			std::size_t mm = m_allocsize + MemIncrement;
			void* mem = std::realloc( m_ar, mm);
			if (!mem) throw std::bad_alloc();
			m_allocsize = mm;
			m_ar = (TYPE**)mem;
		}
		m_ar[ m_size++] = obj;
	}

	void clear()
	{
		for (std::size_t ii=0; ii<m_size; ++ii)
		{
			delete m_ar[ii];
		}
		m_size = 0;
	}

	std::size_t size() const
	{
		return m_size;
	}

	class iterator
	{
	public:
		explicit iterator( TYPE** itr_=0)
			:m_itr(itr_)
		{}
		iterator( const iterator& o)
			:m_itr(o.m_itr){}

		iterator& operator++()					{++m_itr; return *this;}
		iterator operator++(int)				{iterator rt(*this); ++m_itr; return rt;}

		bool operator==( const iterator& o) const		{return m_itr==o.m_itr;}
		bool operator!=( const iterator& o) const		{return m_itr!=o.m_itr;}

		TYPE& operator*()					{return **m_itr;}
		TYPE* operator->()					{return *m_itr;}

	private:
		TYPE** m_itr;
	};

	class const_iterator
	{
	public:
		explicit const_iterator( TYPE* const* itr_=0)
			:m_itr(itr_)
		{}
		const_iterator( const const_iterator& o)
			:m_itr(o.m_itr){}

		const_iterator& operator++()				{++m_itr; return *this;}
		const_iterator operator++(int)				{const_iterator rt(*this); ++m_itr; return rt;}

		bool operator==( const const_iterator& o) const		{return m_itr==o.m_itr;}
		bool operator!=( const const_iterator& o) const		{return m_itr!=o.m_itr;}

		const TYPE& operator*() const				{return **m_itr;}
		const TYPE* operator->() const				{return *m_itr;}

	private:
		TYPE* const* m_itr;
	};

	const_iterator begin() const
	{
		return const_iterator(m_ar);
	}
	iterator begin()
	{
		return iterator(m_ar);
	}
	const_iterator end() const
	{
		return const_iterator(m_ar+m_size);
	}
	iterator end()
	{
		return iterator(m_ar+m_size);
	}

private:
	TYPE** m_ar;
	std::size_t m_allocsize;
	std::size_t m_size;
};

}//namespace
#endif


