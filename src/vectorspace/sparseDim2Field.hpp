/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for a sparse field of dimension 2 (it's a sparse matrix, but we do not provide any operations, just access)
#ifndef _STRUS_VECTOR_SPACE_MODEL_SPARSE_DIM2_FIELD_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SPARSE_DIM2_FIELD_HPP_INCLUDED
#include "strus/base/stdint.h"
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <limits>

namespace strus {

/// \brief Structure for a sparse field of dimension 2
template <typename ValueType>
class SparseDim2Field
{
public:
	struct Coord
	{
		uint32_t x;
		uint32_t y;
	
		Coord() :x(0),y(0){}
		Coord( uint32_t x_, uint32_t y_) :x(x_),y(y_){}
		Coord( const Coord& o) :x(o.x),y(o.y){}
	
		bool operator < (const Coord& o) const
		{
			return (x == o.x)?(y < o.y):(x < o.x);
		}
	};

public:
	SparseDim2Field()				:m_map(){}
	SparseDim2Field( const SparseDim2Field& o)	:m_map(o.m_map){}

	ValueType& operator()( uint32_t x, uint32_t y)
	{
		return m_map[ Coord(x,y)];
	}
	const ValueType& operator()( uint32_t x, uint32_t y) const
	{
		typename std::map<Coord,ValueType>::const_iterator itr = m_map.find( Coord(x,y));
		return itr==m_map.end()?0:itr->second;
	}

	class const_row_iterator
	{
	public:
		const_row_iterator( const typename std::map<Coord,ValueType>::const_iterator& itr_)
			:m_itr(itr_){}
		uint32_t col() const					{return m_itr->first.y;}
		const ValueType& val() const				{return m_itr->second;}

		const_row_iterator& operator++()			{++m_itr; return *this;}
		const_row_iterator operator++(int)			{const_row_iterator rt=m_itr++; return rt;}

		bool operator==( const const_row_iterator& o) const	{return m_itr == o.m_itr;}
		bool operator!=( const const_row_iterator& o) const	{return m_itr != o.m_itr;}

	private:
		typename std::map<Coord,ValueType>::const_iterator m_itr;
	};

	const_row_iterator begin_row( uint32_t x) const
	{
		typename std::map<Coord,ValueType>::const_iterator first =
			(x == 0)?
				 m_map.begin()
				:m_map.upper_bound( Coord(x-1,std::numeric_limits<uint32_t>::max()));
		if (first == m_map.end() || first->first.x != x)
		{
			return m_map.end();
		}
		else
		{
			return first;
		}
	}
	const_row_iterator end_row( uint32_t x) const
	{
		typename std::map<Coord,ValueType>::const_iterator first = m_map.upper_bound( Coord( x, std::numeric_limits<uint32_t>::max()));
		if (first == m_map.end())
		{
			return m_map.end();
		}
		else
		{
			return first;
		}
	}

	std::string tostring() const
	{
		std::ostringstream buf;
		typename std::map<Coord,ValueType>::const_iterator mi = m_map.begin(), me = m_map.end();
		for (; mi != me; ++mi)
		{
			buf << "(" << mi->first.x << "," << mi->first.y << ") = " << mi->second << std::endl;
		}
		return buf.str();
	}

private:
	std::map<Coord,ValueType> m_map;
};

}
#endif
