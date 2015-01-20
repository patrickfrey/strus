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
#ifndef _STRUS_STATUS_HPP_INCLUDED
#define _STRUS_STATUS_HPP_INCLUDED
#include <cstdlib>
#include <cstring>
#include <cstdexcept>

namespace strus
{

/// \brief String for passing accross library borders (only null-terminated, if allocated as copy or if the origin is null-terminated too)
class String
{
public:
	String( const char* ptr_)			:m_free(0){init(ptr_,std::strlen(ptr_));}
	String( const char* ptr_, std::size_t size_)	:m_free(0){init(ptr_,size_);}
	String( const std::string& val)			:m_free(0){init_copy(val.c_str(),val.size());}
	String( char* ptr_)				:m_free(0){init(ptr_,std::strlen(ptr_));}
	String( char* ptr_, std::size_t size_)		:m_free(0){init(ptr_,size_);}
	~String()					{if (m_free)(*m_free)( m_ptr);}

	void init( const char* ptr_, std::size_t size_)
	{
		if (m_free)(*m_free)( m_ptr);
		m_ptr = const_cast<char*>(ptr_);
		m_size = size_;
		m_free = 0;
	}

	void init_copy( char* ptr_, std::size_t size_)
	{
		if (m_free)(*m_free)( m_ptr);
		m_size = size_;
		m_ptr = (char*) std::malloc( size_+1);
		if (!m_ptr) throw 
		std::memcpy( m_ptr, ptr_, size_);
		m_ptr[ size_] = '\0';
		m_free = &std::free;
	}

	void init( char* ptr_, std::size_t size_)
	{
		init_copy( ptr_, size_);
	}

	const char* ptr() const			{return m_ptr;}
	std::size_t size() const		{return m_size;}

private:
	char* m_ptr;
	std::size_t m_size;
	void (*m_free)( char* ptr_);
};

}
#endif
