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
#ifndef _STRUS_LVDB_BLOCK_KEY_HPP_INCLUDED
#define _STRUS_LVDB_BLOCK_KEY_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace strus {
	///\typedef Index
	///\brief Document number type
	typedef INT64 BlockKeyIndex;
}//namespace
#else
#include <stdint.h>
namespace strus {
	///\typedef Index
	///\brief Index term number type
	typedef int64_t BlockKeyIndex;
}//namespace
#endif

namespace strus
{

class BlockKey
{
public:
	BlockKey( const Index& idx1, const Index& idx2)
	{
		if (idx1 <= 0 || idx2 <= 0)
		{
			throw strus::logic_error( _TXT( "using illegal block key [2]"));
		}
		m_index = idx1;
		m_index <<= 32;
		m_index += idx2;
	}
	explicit BlockKey( const Index& idx)
	{
		if (idx <= 0)
		{
			throw strus::logic_error( _TXT( "using illegal block key [1]"));
		}
		m_index = idx;
	}
	BlockKey()
		:m_index(0){}

	explicit BlockKey( const BlockKeyIndex& idx)
	{
		if (idx <= 0)
		{
			throw strus::logic_error( _TXT( "using illegal block key [3]"));
		}
		m_index = idx;
	}

	Index elem( const std::size_t& idx) const
	{
		Index rt;
		if (idx == 1)
		{
			rt = (Index)( m_index >> 32);
			if (!rt) return (Index)m_index;
			return rt;
		}
		else if (idx == 2)
		{
			if ((m_index >> 32) > 0)
			{
				return (Index)(m_index & 0xffFFffFF);
			}
			else
			{
				return 0;
			}
		}
		else
		{
			throw strus::logic_error( _TXT( "illegal block key element access"));
		}
	}

	BlockKeyIndex index() const
	{
		return m_index;
	}

private:
	BlockKeyIndex m_index;
};
}//namespace
#endif
