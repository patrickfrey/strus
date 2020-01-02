/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_STATIC_INTRUSIVE_ARRAY_HPP_INCLUDED
#define _STRUS_STORAGE_STATIC_INTRUSIVE_ARRAY_HPP_INCLUDED

namespace strus {

template <typename Element>
class StaticIntrusiveArray
{
public:
	Element const* ar;
	std::size_t size;

	StaticIntrusiveArray()
		:ar(0),size(0){}
	StaticIntrusiveArray( Element const* ar_, std::size_t size_)
		:ar(ar_),size(size_){}
	StaticIntrusiveArray( const Element& o)
		:ar(o.ar),size(o.size){}
	StaticIntrusiveArray& operator=( const StaticIntrusiveArray& o)
		{ar=o.ar; size=o.size; return *this;}

	void init( Element const* ar_, std::size_t size_)
	{
		ar = ar_;
		size = size_;
	}

	const Element& operator[]( int idx) const
	{
		return ar[ idx];
	}

	const Element& operator[]( std::size_t idx) const
	{
		return ar[ idx];
	}
};

}//namespace
#endif

