/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Numeric types used for local and global indices
/// \file "index.hpp"
#ifndef _STRUS_INDEX_HPP_INCLUDED
#define _STRUS_INDEX_HPP_INCLUDED

#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace strus {
	///\typedef Index
	///\brief Number type generally used for locally counted indices
	typedef INT32 Index;
	///\typedef GlobalCounter
	///\brief Number type generally used for indices globally shared between different instances of strus
	typedef INT64 GlobalCounter;
}//namespace
#else
#include <stdint.h>
namespace strus {
	///\typedef Index
	///\brief Number type generally used for locally counted indices
	typedef int32_t Index;
	///\typedef GlobalCounter
	///\brief Number type generally used for indices globally shared between different instances of strus
	typedef int64_t GlobalCounter;
}//namespace
#endif

namespace strus {

class IndexRange
{
public:
	Index start() const
	{
		return m_start;
	}
	Index end() const
	{
		return m_end;
	}
	int len() const
	{
		return m_end - m_start;
	}
	bool defined() const
	{
		return m_start < m_end;
	}

	IndexRange()
		:m_start(0),m_end(0){}
	IndexRange( Index start_, Index end_)
		:m_start(start_),m_end(end_){}
	IndexRange( const IndexRange& o)
		:m_start(o.m_start),m_end(o.m_end){}

	bool operator < (const IndexRange& o) const
	{
		return (m_end == o.m_end) ? (m_start < o.m_start) : (m_end < o.m_end);
	}
	bool operator > (const IndexRange& o) const
	{
		return (m_end == o.m_end) ? (m_start > o.m_start) : (m_end > o.m_end);
	}
	bool operator == (const IndexRange& o) const
	{
		return (m_end == o.m_end && m_start == o.m_start);
	}

private:
	Index m_start;
	Index m_end;
};

}//namespace
#endif

