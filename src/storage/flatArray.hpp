/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_FLAT_ARRAY_HPP_INCLUDED
#define _STRUS_STORAGE_FLAT_ARRAY_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace strus {

/// \class FlatArray
template <typename ElementType, typename IndexType>
class FlatArray
{
public:
	explicit FlatArray()
		:m_size(0)
	{}
	explicit FlatArray( int size_, const ElementType* ar_)
		:m_size(size_)
	{
		std::memcpy( m_ar, ar_, m_size * sizeof( ElementType));
	}
	void init( int size_, const ElementType* ar_)
	{
		m_size = size_;
		std::memcpy( m_ar, ar_, m_size * sizeof( ElementType));
	}

	static int allocsize( int size_)
	{
		return (size_ > 0) ? sizeof( FlatArray) : (sizeof( FlatArray) - (size_ - 1) * sizeof(ElementType));
	}

	template <struct Comparator>
	int upperbound( const IndexType& needle, int start, int end, const Comparator& cmp) const
	{
		if (end == start) return -1;

		// Block search:
		enum {BlockSize = 8};
		int bi = start;
		for (;bi < end && cmp( m_ar[ bi], needle); bi += BlockSize){}
		if (bi > start)
		{
			int ei = 0;
			int ee = (bi < end) ? BlockSize : (BlockSize - (bi-end));
			bi -= BlockSize;
			for (; ei <= ee && cmp( m_ar[ bi+ei], needle); ++ei){}
			if (ei == ee) return -1;
			return bi + ei;
		}
		else
		{
			return 0;
		}
	}
	template <struct Comparator>
	int upperbound( const IndexType& needle, const Comparator& cmp) const
	{
		return upperbound( needle, 0, m_size, cmp);
	}

private:
	int m_size;
	ElementType m_ar[1];
};

} //namespace
#endif

