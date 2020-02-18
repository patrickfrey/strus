/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BLOCK_KEY_HPP_INCLUDED
#define _STRUS_STORAGE_BLOCK_KEY_HPP_INCLUDED
#include "strus/storage/index.hpp"
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
			throw std::runtime_error( _TXT( "using 0 in block key [2]"));
		}
		m_index = idx1;
		m_index <<= 32;
		m_index += idx2;
	}
	explicit BlockKey( const Index& idx)
	{
		if (idx <= 0)
		{
			throw std::runtime_error( _TXT( "using 0 in block key [1]"));
		}
		m_index = idx;
	}
	BlockKey()
		:m_index(0){}

	explicit BlockKey( const BlockKeyIndex& idx)
	{
		if (idx <= 0)
		{
			throw std::runtime_error( _TXT( "using 0 in block key [3]"));
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
			throw std::runtime_error( _TXT( "internal: illegal block key element access"));
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
