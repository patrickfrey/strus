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
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace strus
{

/// \brief Reference for passing objects accross library borders.
template <class Object>
class Reference
{
private:
	static void destroy( Object* obj_)
	{
		delete obj_;
	}

public:
	Reference()
		:m_free(0),m_obj(0),m_refcnt(newRefCnt(0)){}
	Reference( Object* obj_)
		:m_free(&destroy),m_obj(obj_),m_refcnt(newRefCnt(1)){}
	Reference( const Reference& o)
		:m_free(o.m_free),m_obj(o.m_obj),m_refcnt(o.m_refcnt){++*m_refcnt;}

	~Reference()
	{
		freeRef();
	}

	Reference& operator = (const Reference& o)
	{
		m_free = o.m_free;
		m_obj = o.m_obj;
		m_refcnt = o.m_refcnt;
		++*m_refcnt;
		return *this;
	}

	void reset( Object* obj_)			
	{
		if (*m_refcnt == 1)
		{
			freeRef();
		}
		else
		{
			int* rc = newRefCnt(1);
			freeRef();
			m_refcnt = rc;
		}
		m_free = &destroy;
		m_obj = obj_;
	}

	Object* operator->()				{return m_obj;}
	const Object* operator->() const		{return m_obj;}
	Object& operator*()				{return *m_obj;}
	const Object& operator*() const			{return *m_obj;}

	const Object* get() const			{return m_obj;}
	Object* get()					{return m_obj;}

private:
	int* newRefCnt( int initval=0)
	{
		int* rt = (int*)std::malloc(sizeof(int));
		if (!rt) throw std::bad_alloc();
		*rt = initval;
		return rt;
	}
	void freeRef()
	{
		if (--*m_refcnt == 0 && m_free)(*m_free)( m_obj);
	}

private:
	void (*m_free)( Object* ptr_);
	Object* m_obj;
	mutable int* m_refcnt;
};

}
#endif

