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
#ifndef _STRUS_STORAGE_BYTE_ORDER_MARK_HPP_INCLUDED
#define _STRUS_STORAGE_BYTE_ORDER_MARK_HPP_INCLUDED
#include "strus/index.hpp"
#include <cstring>

namespace strus {

class ByteOrderMark
{
public:
	ByteOrderMark()
	{
		std::memset( this, 0, sizeof(*this));
		for (int ii=0; ii<4; ++ii) data.ar[ii] = ii;
	}

	void set( const Index& val)
	{
		data.val = val;
	}

	Index value() const
	{
		return data.val;
	}

	const char* endianess() const
	{
		if (data.val == 0x01020304)
		{
			return "big endian";
		}
		if (data.val == 0x04030201)
		{
			return "little endian";
		}
		return "endianess unknown (corrupt data)";
	}

private:
	union
	{
		Index val;
		char ar[4];
	} data;
};

} //namespace
#endif

