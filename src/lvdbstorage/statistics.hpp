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
#ifndef _STRUS_LVDB_STATISTICS_HPP_INCLUDED
#define _STRUS_LVDB_STATISTICS_HPP_INCLUDED
#include <stdint.h>
#include <boost/atomic.hpp>
#include <iostream>

#define _STRUS_ENABLE_STATISTICS

namespace strus {

struct Statistics
{
	typedef unsigned int ValueType;

	enum Type
	{
		MetaDataCacheMiss,
		DocnoBlockReadBlockRandom,
		DocnoBlockReadBlockRandomMiss,
		DocnoBlockReadBlockFollow,
		DocnoBlockReadBlockFollowMiss,
		PosinfoBlockReadBlockRandom,
		PosinfoBlockReadBlockRandomMiss,
		PosinfoBlockReadBlockFollow,
		PosinfoBlockReadBlockFollowMiss
	};
	enum {NofTypes=PosinfoBlockReadBlockFollowMiss+1};

	static const char* typeName( Type i)
	{
		static const char* ar[NofTypes] = {
			"MetaDataCacheMiss",
			"DocnoBlockReadBlockRandom",
			"DocnoBlockReadBlockRandomMiss",
			"DocnoBlockReadBlockFollow",
			"DocnoBlockReadBlockFollowMiss",
			"PosinfoBlockReadBlockRandom",
			"PosinfoBlockReadBlockRandomMiss",
			"PosinfoBlockReadBlockFollow",
			"PosinfoBlockReadBlockFollowMiss"
		};
		return ar[i];
	}

#ifdef _STRUS_ENABLE_STATISTICS
	static void increment( Type idx);
	static ValueType value( Type idx);
#else
	static void increment( Type){}
	static ValueType value( Type) {return 0;}
#endif

	class const_iterator
	{
	public:
		const_iterator( int idx_)
			:m_idx(idx_){}

		const_iterator& operator++()			{incr(); return *this;}
		const_iterator operator++(int)			{const_iterator rt(m_idx);incr(); return rt;}

		bool operator==( const const_iterator& o) const	{return m_idx == o.m_idx;}
		bool operator!=( const const_iterator& o) const	{return m_idx != o.m_idx;}

		const char* typeName() const			{return Statistics::typeName( (Type)m_idx);}
		Type type() const				{return (Type)m_idx;}
		ValueType value() const				{return Statistics::value( (Type)m_idx);}

	private:
		void incr()
		{
			if (m_idx < NofTypes)
			{
				++m_idx;
			}
			else
			{
				m_idx = NofTypes;
			}
		}

	private:
		int m_idx;
	};

	static const_iterator begin()
	{
#ifdef _STRUS_ENABLE_STATISTICS
		return const_iterator(0);
#else
		return end();
#endif
	}
	static const_iterator end()
	{
		return const_iterator(NofTypes);
	}
};

}//namespace
#endif



