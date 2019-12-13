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
#include <algorithm>

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
	SkipScanArray( const SkipScanArray& o)
		:m_ar(o.m_ar),m_size(o.m_size)
	{}
	~SkipScanArray(){}
	SkipScanArray& operator=( const SkipScanArray& o)
		{m_ar=o.m_ar; m_size=o.m_size; return *this;}

	void init( const ElementType* ar_, int size_)
	{
		m_ar = ar_;
		m_size = size_;
	}

	int upperbound( const IndexType& needle, int start, int end, const ComparatorFunctor& cmp) const
	{
		enum {NotFound=-1};

		// Block search (fibonacci):
		int fib1 = 1, fib2 = 1;
		int bi = start;
		while (bi < end && cmp( m_ar[ bi], needle))
		{
			bi = start + fib1;
			fib2 = fib1 + fib2;
			std::swap( fib1, fib2);
		}
		if (fib1 - fib2 > 1)
		{
			int ei = start + fib2;
			int ee = start + fib1;
			if (end > ee) ee = end;
			for (; ei < ee && cmp( m_ar[ start+ei], needle); ++ei){}
			return (ei == end) ? NotFound : ei;
		}
		else
		{
			return bi == end ? NotFound : bi;
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

