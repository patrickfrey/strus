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
/// \brief Variant type for fixed size numeric types (integers, floating point numbers)
/// \file "arithmeticVariant.hpp"
#ifndef _STRUS_LVDB_ARITHMETIC_VARIANT_TYPE_HPP_INCLUDED
#define _STRUS_LVDB_ARITHMETIC_VARIANT_TYPE_HPP_INCLUDED
#include <cstring>
#include <limits>

namespace strus {


/// \class ArithmeticVariant
/// \brief Atomic type that can hold numeric values of different type
class ArithmeticVariant
{
public:
	/// \brief Constructor from a signed integer
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant( int value)
	{
		variant.Int = value;
		type = Int;
	}

	/// \brief Constructor from an unsigned integer
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
	}

	/// \brief Constructor from a single precision floating point number
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant( float value)
	{
		variant.Float = value;
		type = Float;
	}

	/// \brief Default constructor (as undefined value)
	ArithmeticVariant()
	{
		std::memset( this, 0, sizeof(*this));
	}

	/// \brief Copy constructor 
	/// \param[in] o arithmetic variant to copy
	ArithmeticVariant( const ArithmeticVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}

	/// \brief Find out if this value is defined
	/// \return true, if yes
	bool defined() const
	{
		return type != Null;
	}

	/// \brief Template for casting to a defined value type
	/// \tparam TYPE what type to cast this arithmetic variant to
	template <typename TYPE>
	TYPE cast() const
	{
		switch (type)
		{
			case Null: return TYPE();
			case Int: return (TYPE)variant.Int;
			case UInt: return (TYPE)variant.UInt;
			case Float: return (TYPE)variant.Float;
		}
		return TYPE();
	}

	/// \brief Cast to a single precision floating point number
	operator float() const
	{
		return cast<float>();
	}
	/// \brief Cast to a signed integer
	operator int() const
	{
		return cast<int>();
	}
	/// \brief Cast to an unsigned integer
	operator unsigned int() const
	{
		return cast<unsigned int>();
	}

	/// \brief Test for equality
	/// \param[in] o arithmetic variant to compare
	/// \return true, if yes
	bool operator == (const ArithmeticVariant& o) const
	{
		return isequal(o);
	}
	/// \brief Test for inequality
	/// \param[in] o arithmetic variant to compare
	/// \return true, if yes
	bool operator != (const ArithmeticVariant& o) const
	{
		return !isequal(o);
	}

	/// \brief Test for equality
	/// \param[in] o arithmetic variant to compare
	/// \return true, if yes
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

	/// \brief Assignment operator for a singed integer
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant& operator=( int value)
	{
		variant.Int = value;
		type = Int;
		return *this;
	}

	/// \brief Assignment operator for an unsinged integer
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant& operator=( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
		return *this;
	}

	/// \brief Assignment operator for a single precision floating point number
	/// \param[in] value value to assign to this arithmetic variant
	ArithmeticVariant& operator=( float value)
	{
		variant.Float = value;
		type = Float;
		return *this;
	}

	/// \brief Assignment operator
	/// \param[in] o arithmetic variant to copy
	ArithmeticVariant& operator=( const ArithmeticVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
		return *this;
	}

	/// \brief Enumeration of all types an arithmetic variant can have
	enum Type {
		Null,		///< uninitialized variant value
		Int,		///< signed integer number value
		UInt,		///< unsigned integer number value
		Float		///< floating point number value
	};
	Type type;				///< Type of this arithmetic variant
	union
	{
		int Int;			///< value in case of a signed integer
		unsigned int UInt;		///< value in case of an unsigned integer
		float Float;			///< value in case of a single precision floating point number
	} variant;				///< Value of this arithmetic variant
};

}//namespace
#endif
