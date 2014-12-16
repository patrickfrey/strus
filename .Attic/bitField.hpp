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
#ifndef _STRUS_LVDB_BITFIELD_HPP_INCLUDED
#define _STRUS_LVDB_BITFIELD_HPP_INCLUDED
#include <utility>
#include <cstrings>

namespace strus {

class BitField
{
public:
	BitField()
		:m_ar(0),m_size(0){}
	BitField( const BitField& o)
		:m_ar(o.m_ar),m_size(o.m_size){}
	BitField( void* ar_, std::size_t bitsize_)
		:m_ar((unsigned long*)ar_),m_size(bitsize_/(8*sizeof(m_ar[0]))){}

	bool operator[]( std::size_t idx) const
	{
		std::size_t ii = idx/(8*sizeof(m_ar[0]));
		std::size_t bb = idx%(8*sizeof(m_ar[0]));
		if (ii >= m_size) throw std::logic_error("array bound read (BitField)");
		return 0!=(m_ar[ ii] & (1<<bb));
	}

	void set( std::size_t idx, bool value=true)
	{
		std::size_t ii = idx/(8*sizeof(m_ar[0]));
		std::size_t bb = idx%(8*sizeof(m_ar[0]));
		if (ii >= m_size) throw std::logic_error("array bound write (BitField)");
		if (value)
		{
			m_ar[ ii] |= (1<<bb);
		}
		else
		{
			m_ar[ ii] &= ~(1<<bb);
		}
	}

	std::size_t next( std::size_t idx)
	{
		std::size_t ii = idx/(8*sizeof(m_ar[0]));
		std::size_t bb = idx%(8*sizeof(m_ar[0]));
		if (ii >= m_size) throw std::logic_error("array bound read (BitField)");
		unsigned long val = m_ar[ ii] & ~(1<<bb);
		std::size_t pp = ffsl( val);
		while (!pp && ++ii<m_size)
		{
			pp = ffsl( m_ar[ii]);
		}
		return (ii<m_size) ? (ii * (8*sizeof(m_ar[0])) + pp):0;
	}

private:
	unsigned long* m_ar;
	std::size_t m_size;
};

}//namespace
#endif

