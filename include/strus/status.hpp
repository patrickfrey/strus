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
#include <cstring>
#include <cstdlib>

namespace strus
{

/// \brief Success or error state object
class Status
{
public:
	enum Id
	{
		Ok,
		Error,
		OutOfMem
	};

	Status()
		:m_id(Ok)
	{
		m_errorstring[0] = '\0';
	}

	Status( const char* msg)
		:m_id(Ok)
	{
		std::size_t nn = std::strlen( msg);
		if (nn >= ErrorStringBufSize) nn = ErrorStringBufSize-1;
		std::memcpy( m_errorstring, msg, nn);
		m_errorstring[ nn] = '\0';
	}

	static Status outOfMem()
	{
		Status rt;
		rt.m_id = OutOfMem;
		return rt;
	}

	const bool ok() const
	{
		return m_id == Ok;
	}

	const bool id() const
	{
		return m_id;
	}

	const char* error() const
	{
		return &m_errorstring;
	}

private:
	enum {ErrorStringBufSize=256};
	Id m_id;
	char m_errorstring[ ErrorStringBufSize];
};

}
#endif

