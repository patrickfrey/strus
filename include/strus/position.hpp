/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_DOCNUM_HPP_INCLUDED
#define _STRUS_DOCNUM_HPP_INCLUDED

#ifdef BOOST_VERSION
#include <boost/cstdint.hpp>
namespace strus {
	///\typedef Index
	///\brief Index term number type
	typedef boost::int64_t Index;
}//namespace
#else
#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace strus {
	///\typedef Index
	///\brief Document number type
	typedef DWORD64 Index;
}//namespace
#else
#include <stdint.h>
namespace strus {
	///\typedef Index
	///\brief Index term number type
	typedef int64_t Index;
}//namespace
#endif
#endif

namespace strus {

typedef Index Position;
typedef Index DocPosition;
typedef Index DocNumber;
typedef Index TermNumber;

enum {DocPositionShift=24, MaxDocPosition=(DocPositionShift-1)};

struct Encode
{
	static DocPosition getDocPosition( Position pos)
	{
		return pos & 0xFFFFffU;
	}
	static DocNumber getDocNumber( Position pos)
	{
		return pos >> DocPositionShift;
	}
	static Position getPosition( DocNumber docnum, DocPosition docpos)
	{
		return (docnum << DocPositionShift) + docpos;
	}
	static unsigned int getErrorCode( int component, unsigned int err)
	{
		return (component * 0x10000 + err);
	}
};

}//namespace
#endif

