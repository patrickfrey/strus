/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Variant type for fixed size numeric types (integers, floating point numbers)
/// \file numericVariant.hpp
#ifndef _STRUS_STORAGE_NUMERIC_VARIANT_TYPE_HPP_INCLUDED
#define _STRUS_STORAGE_NUMERIC_VARIANT_TYPE_HPP_INCLUDED
#include <cstring>
#include <stdio.h>
#include <limits>

namespace strus {


/// \class NumericVariant
/// \brief Atomic type that can hold numeric values of different type
class NumericVariant
{
public:
	/// \brief Constructor from a signed integer
	/// \param[in] value value to assign to this numeric variant
	NumericVariant( int value)
	{
		variant.Int = value;
		type = Int;
	}

	/// \brief Constructor from an unsigned integer
	/// \param[in] value value to assign to this numeric variant
	NumericVariant( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
	}

	/// \brief Constructor from a single precision floating point number
	/// \param[in] value value to assign to this numeric variant
	NumericVariant( double value)
	{
		variant.Float = value;
		type = Float;
	}

	/// \brief Default constructor (as undefined value)
	NumericVariant()
	{
		init();
	}

	/// \brief Copy constructor 
	/// \param[in] o numeric variant to copy
	NumericVariant( const NumericVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
	}

	void init()
	{
		std::memset( this, 0, sizeof(*this));
	}

	class String
	{
	public:
		String()
		{
			m_buf[0] = '\0';
		}

		String( const NumericVariant& val)
		{
			switch (val.type)
			{
				case Null: break;
				case Int: snprintf( m_buf, sizeof(m_buf), "%d", val.variant.Int); return;
				case UInt: snprintf( m_buf, sizeof(m_buf), "%u", val.variant.UInt); return;
				case Float: snprintf( m_buf, sizeof(m_buf), "%.5f", val.variant.Float); return;
			}
			m_buf[0] = '\0';
		}

		operator const char*() const	{return m_buf;}
		const char* c_str() const	{return m_buf;}

	private:
		char m_buf[ 128];
	};

	String tostring() const
	{
		return String( *this);
	}

	bool initFromString( const char* src)
	{
		char const* si = src;
		bool sign_ = false;
		if (!*si)
		{
			init();
			return true;
		}
		if (*si && *si == '-')
		{
			sign_ = true;
			++si;
		}
		if (*si < '0' || *si > '9') return false;
		for (; *si >= '0' && *si <= '9'; ++si){}
		if (*si == '.')
		{
			for (++si; *si >= '0' && *si <= '9'; ++si){}
			if (*si) return false;
			sscanf( src, "%lf", &variant.Float);
			type = Float;
		}
		else if (sign_)
		{
			sscanf( src, "%d", &variant.Int);
			type = Int;
		}
		else
		{
			sscanf( src, "%u", &variant.UInt);
			type = UInt;
		}
		return true;
	}

	/// \brief Find out if this value is defined
	/// \return true, if yes
	bool defined() const
	{
		return type != Null;
	}

	/// \brief Template for casting to a defined value type
	/// \tparam TYPE what type to cast this numeric variant to
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
	operator double() const
	{
		return cast<double>();
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

	/// \brief Cast to a signed integer
	int toint() const
	{
		return cast<int>();
	}

	/// \brief Cast to an unsigned integer
	unsigned int touint() const
	{
		return cast<unsigned int>();
	}

	/// \brief Cast to an unsigned integer
	double tofloat() const
	{
		return cast<double>();
	}

	/// \brief Test for equality
	/// \param[in] o numeric variant to compare
	/// \return true, if yes
	bool operator == (const NumericVariant& o) const
	{
		return isequal(o);
	}
	/// \brief Test for inequality
	/// \param[in] o numeric variant to compare
	/// \return true, if yes
	bool operator != (const NumericVariant& o) const
	{
		return !isequal(o);
	}

	/// \brief Test for equality
	/// \param[in] o numeric variant to compare
	/// \return true, if yes
	bool isequal( const NumericVariant& o) const
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
					double xx = variant.Float - o.variant.Float;
					if (xx < 0) xx = -xx;
					return xx <= std::numeric_limits<double>::epsilon();
				}
			}
		}
		return false;
	}

	/// \brief Assignment operator for a singed integer
	/// \param[in] value value to assign to this numeric variant
	NumericVariant& operator=( int value)
	{
		variant.Int = value;
		type = Int;
		return *this;
	}

	/// \brief Assignment operator for an unsinged integer
	/// \param[in] value value to assign to this numeric variant
	NumericVariant& operator=( unsigned int value)
	{
		variant.UInt = value;
		type = UInt;
		return *this;
	}

	/// \brief Assignment operator for a single precision floating point number
	/// \param[in] value value to assign to this numeric variant
	NumericVariant& operator=( double value)
	{
		variant.Float = value;
		type = Float;
		return *this;
	}

	/// \brief Assignment operator
	/// \param[in] o numeric variant to copy
	NumericVariant& operator=( const NumericVariant& o)
	{
		std::memcpy( this, &o, sizeof(*this));
		return *this;
	}

	/// \brief Enumeration of all types an numeric variant can have
	enum Type {
		Null,		///< uninitialized variant value
		Int,		///< signed integer number value
		UInt,		///< unsigned integer number value
		Float		///< floating point number value
	};
	Type type;				///< Type of this numeric variant
	union
	{
		int Int;			///< value in case of a signed integer
		unsigned int UInt;		///< value in case of an unsigned integer
		double Float;			///< value in case of a floating point number
	} variant;				///< Value of this numeric variant
};

}//namespace
#endif
