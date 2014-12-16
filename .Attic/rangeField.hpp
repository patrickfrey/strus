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
#ifndef _STRUS_LVDB_RANGE_FIELD_HPP_INCLUDED
#define _STRUS_LVDB_RANGE_FIELD_HPP_INCLUDED
#include <utility>
#include <cstrings>

namespace strus {

class RangeField
{
public:
	RangeField()
		:m_ar(0),m_size(0){}
	RangeField( const RangeField& o)
		:m_ar(o.m_ar),m_size(o.m_size){}
	RangeField( void* ar_, std::size_t bytesize_)
		:m_ar((Index*)ar_),m_size(bytesize_/(sizeof(m_ar[0]))){}

	Index upper_bound( const Index& idx)
	{
		std::size_t itr = 0;
		return upper_bound( idx, itr);
	}

	Index upper_bound( const Index& idx, std::size_t& itr)
	{
		if (get_upper_bound_idx( itr, idx, itr))
		{
			Index start = rangeStart( itr);
			Index end = rangeEnd( itr);
			if (start >= itr)
			{
				return start;
			}
			return itr;
		}
		else
		{
			return 0;
		}
	}

	static std::size_t packRange( Index* dest, Index from_, Index to_)
	{
		if (to_ - from_ == 0)
		{
			dest[0] = from_;
			return sizeof( Index);
		}
		else if (to_ - from_ > 1)
		{
			dest[0] = to_;
			dest[1] = from_ - to_;
			return sizeof( Index);
		}
		else
		{
			throw std::logic_error( "illegal range (RangeField::packRange)");
		}
	}

	bool getTopRange( Index& from_, Index& to_)
	{
		if (m_size == 0) return false;
		from_ = m_ar[ m_size-1];
		if (from_ < 0)
		{
			to_ = m_ar[ m_size-2];
			from_ = to_ + from_;
		}
		return true;
	}

	bool incrTopRange()
	{
		if (m_size < 2) return false;
		if (m_ar[ m_size-1] > 0) throw std::logic_error("invalid operation (rangeField::incrTopRange)");
		m_ar[ m_size-1] -= 1;
		m_ar[ m_size-2] += 1;
		return true;
	}

private:
	bool get_upper_bound_idx( std::size_t& res, const Index& idx, std::size_t hint=0)
	{
		if (hint >= m_size || m_ar[hint] < 0 || m_ar[hint] > idx)
		{
			if (!m_size) return false;
			hint = 0;
		}
		std::size_t first = 0, last = m_size-1, mid = hint;
		if (rangeEnd(last) < idx)
		{
			return false;
		}
		if (rangeStart(first) >= idx)
		{
			res = first;
			return true;
		}
		Index last_start = rangeStart( last);
		Index first_end = rangeEnd( first);
		Index mid_start = rangeStart( mid);
		Index mid_end = rangeEnd( mid);
		do
		{
			if (mid_start > idx)
			{
				last = mid;
				last_start = mid_start;
			}
			else
			{
				if (mid_end >= idx)
				{
					res = mid;
					return true;
				}
				first_end = mid_end;
				first = mid;
			}
			mid = (last + first) /2;
			mid_start = rangeStart( mid);
			mid_end = rangeEnd( mid);
		}
		while (last - first > 4);

		while (first <= last && rangeEnd(first) < idx)
		{
			++first;
			if (first < m_size && m_ar[ first] < 0) ++first;
		}
		if (m_ar[first] < 0)
		{
			--first;
		}
		res = first;
		return true;
	}


private:
	Index rangeStart( const std::size_t& idx)
	{
		Index rt = m_ar[ idx];
		if (rt < 0)
		{
			rt += m_ar[ idx-1];
		}
		else
		{
			if (idx+1 < m_size && m_ar[ idx+1] < 0)
			{
				rt += m_ar[ idx+1];
			}
		}
		return rt;
	}
	Index rangeEnd( const std::size_t& idx)
	{
		Index rt = m_ar[ idx];
		if (rt < 0)
		{
			rt = m_ar[ idx-1];
		}
		return rt;
	}

private:
	Index* m_ar;
	std::size_t m_size;
};

}//namespace
#endif

