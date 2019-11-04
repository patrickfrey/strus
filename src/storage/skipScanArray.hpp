/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_SKIP_SCAN_ARRAY_HPP_INCLUDED
#define _STRUS_STORAGE_SKIP_SCAN_ARRAY_HPP_INCLUDED
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace strus {

/// \class SkipScanArray
template <typename ElementType, typename IndexType, class ComparatorFunctor>
class SkipScanArray
{
public:
	explicit SkipScanArray()
		:m_ar(0),m_size(0)
	{}
	explicit SkipScanArray( const ElementType* ar_, int size_)
		:m_ar(ar_),m_size(size_)
	{}
	~SkipScanArray(){}

	void init( const ElementType* ar_, int size_)
	{
		m_ar = ar_;
		m_size = size_;
	}

	int upperbound( const IndexType& needle, int start, int end, const ComparatorFunctor& cmp) const
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
			for (; ei < ee && cmp( m_ar[ bi+ei], needle); ++ei){}
			if (bi+ei == end) return -1/*upperbound not found*/;
			return bi + ei;
		}
		else
		{
			return 0/*upperbound is first element*/;
		}
	}

	int upperbound( const IndexType& needle, const ComparatorFunctor& cmp) const
	{
		return upperbound( needle, 0, m_size, cmp);
	}

	const ElementType* at( int idx) const
	{
		return m_ar + idx;
	}
	const ElementType& operator[]( int idx) const
	{
		return m_ar[ idx];
	}
	int size() const
	{
		return m_size;
	}

private:
	const ElementType* m_ar;
	int m_size;
};

} //namespace
#endif

