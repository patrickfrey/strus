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
/// \brief Shared reference of an object that is not shared between different threads.
/// \file "reference.hpp"
#ifndef _STRUS_REFERENCE_HPP_INCLUDED
#define _STRUS_REFERENCE_HPP_INCLUDED
#include <cstdlib>
#include <stdexcept>
#include <new>

namespace strus
{

/// \brief Shared pointer template with non thread-safe reference counting.
/// \note Similar to shared_ptr but without atomic (thread safe) reference counting
template <class Object>
class Reference
{
public:
	/// \brief Default constructor
	Reference()
		:m_obj(0),m_refcnt(0){}
	/// \brief Constructor
	Reference( Object* obj_)
		:m_obj(0),m_refcnt(0)
	{
		try
		{
			m_refcnt = newRefCnt();
			m_obj = obj_;
		}
		catch (const std::bad_alloc&)
		{
			delete m_obj;
		}
	}
	/// \brief Copy constructor
	Reference( const Reference& o)
		:m_obj(o.m_obj),m_refcnt(o.m_refcnt)
	{
		if (m_refcnt) ++*m_refcnt;
	}

	/// \brief Destructor
	~Reference()
	{
		freeRef();
	}

	/// \brief Assignment operator
	Reference& operator = (const Reference& o)
	{
		m_obj = o.m_obj;
		m_refcnt = o.m_refcnt;
		if (m_refcnt) ++*m_refcnt;
		return *this;
	}

	/// \brief Reinitialize the local value of the reference and dispose the old value if not referenced by others
	void reset( Object* obj_=0)
	{
		if (!m_refcnt)
		{
			if (obj_)
			{
				m_refcnt = newRefCnt();
			}
		}
		else if (*m_refcnt == 1)
		{
			delete m_obj;
		}
		else if (obj_)
		{
			int* rc = newRefCnt();
			freeRef();
			m_refcnt = rc;
		}
		else
		{
			freeRef();
			m_refcnt = 0;
		}
		m_obj = obj_;
	}

	/// \brief Object access operator
	Object* operator->()				{return m_obj;}
	/// \brief Object access operator
	const Object* operator->() const		{return m_obj;}
	/// \brief Object access operator
	Object& operator*()				{return *m_obj;}
	/// \brief Object access operator
	const Object& operator*() const			{return *m_obj;}

	/// \brief Object access as function
	const Object* get() const			{return m_obj;}
	/// \brief Object access as function
	Object* get()					{return m_obj;}

	/// \brief Release object reference and return the pointer to the object (with ownership) if this is possible
	/// \remark A multiply shared object reference cannot be released
	Object* release()
	{
		if (m_refcnt && *m_refcnt == 1)
		{
			--*m_refcnt;
			Object* rt = m_obj;
			std::free( m_refcnt);
			m_refcnt = 0;
			m_obj = 0;
			return rt;
		}
		else
		{
			throw std::logic_error( "cannot release shared object (having more than one reference)");
		}
	}

private:
	int* newRefCnt()
	{
		int* rt = (int*)std::malloc(sizeof(int));
		if (!rt) throw std::bad_alloc();
		*rt = 1;
		return rt;
	}
	void freeRef()
	{
		if (m_refcnt && --*m_refcnt == 0)
		{
			delete m_obj;
			std::free( m_refcnt);
			m_refcnt = 0;
			m_obj = 0;
		}
	}

private:
	Object* m_obj;
	mutable int* m_refcnt;
};

}
#endif

