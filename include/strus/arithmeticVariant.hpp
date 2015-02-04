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
#ifndef _STRUS_LVDB_ARITHMETIC_VARIANT_TYPE_HPP_INCLUDED
#define _STRUS_LVDB_ARITHMETIC_VARIANT_TYPE_HPP_INCLUDED
#include <cstring>
#include <stdexcept>
#include <limits>

namespace strus {

struct ArithmeticVariant
{
	ArithmeticVariant( int value)
	{
		variant.Int = value;
		type = Int;
	}

	ArithmeticVariant( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
	}

	ArithmeticVariant( float value)
	{
		variant.Float = value;
		type = Float;
	}

	ArithmeticVariant()
	{
		std::memset( this, 0, sizeof(*this));
	}

	ArithmeticVariant( const ArithmeticVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}

	bool defined() const
	{
		return type != Null;
	}

	template <typename TYPE>
	TYPE cast() const
	{
		switch (type)
		{
			case Null: throw std::logic_error( "illegal cast of NULL");
			case Int: return (TYPE)variant.Int;
			case UInt: return (TYPE)variant.UInt;
			case Float: return (TYPE)variant.Float;
		}
		throw std::logic_error( "illegal value of variant");
	}

	operator float() const
	{
		return cast<float>();
	}
	operator int() const
	{
		return cast<int>();
	}
	operator unsigned int() const
	{
		return cast<unsigned int>();
	}

	bool operator == (const ArithmeticVariant& o) const
	{
		return isequal(o);
	}
	bool operator != (const ArithmeticVariant& o) const
	{
		return !isequal(o);
	}

	bool isequal( const ArithmeticVariant& o) const
	{
		if (type == o.type)
		{
			switch (type)
			{
				case Null: return true;
				case Int: return variant.Int == o.variant.Int;
				case UInt: return variant.UInt == o.variant.UInt;
				case Float:
				{
					float xx = variant.Float - o.variant.Float;
					if (xx < 0) xx = -xx;
					return xx <= std::numeric_limits<float>::epsilon();
				}
			}
		}
		return false;
	}

	ArithmeticVariant& operator=( int value)
	{
		variant.Int = value;
		type = Int;
		return *this;
	}

	ArithmeticVariant& operator=( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
		return *this;
	}

	ArithmeticVariant& operator=( float value)
	{
		variant.Float = value;
		type = Float;
		return *this;
	}

	ArithmeticVariant& operator=( const ArithmeticVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
		return *this;
	}

	enum Type {Null,Int,UInt,Float};
	Type type;
	union
	{
		int Int;
		unsigned int UInt;
		float Float;
	} variant;
};

}//namespace
#endif
