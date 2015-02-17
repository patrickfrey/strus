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
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/dll_tags.hpp"
#include <sstream>
#include <boost/lexical_cast.hpp>

using namespace strus;

static inline bool isDigit( char ch)
{
	return (ch >= '0' && ch <= '9');
}

DLL_PUBLIC ArithmeticVariant strus::arithmeticVariantFromString( const std::string& valueAsString)
{
	std::string::const_iterator vi = valueAsString.begin(), ve = valueAsString.end();
	bool sign = false;
	unsigned int ival = 0;
	unsigned int pval = 0;
	float fval = 0.0;
	bool int_overflow = false;
	if (vi == ve)
	{
		throw std::runtime_error( "empty string cannot be converted to arithmetic variant");
	}
	if (vi != ve && *vi == '-')
	{
		++vi;
		sign = true;
	}
	for (; vi != ve; ++vi)
	{
		if (!isDigit( *vi)) break;
		ival = ival * 10 + *vi - '0';
		fval = fval * 10 + *vi - '0';
		if (pval > ival)
		{
			int_overflow = true;
		}
		pval = ival;
	}
	if (vi == ve)
	{
		if (int_overflow)
		{
			return ArithmeticVariant( sign?fval:-fval);
		}
		if (sign) 
		{
			if (ival > (unsigned int)std::numeric_limits<int>::max())
			{
				return ArithmeticVariant( sign?fval:-fval);
			}
			return ArithmeticVariant( -(int)ival);
		}
		else
		{
			return ArithmeticVariant( ival);
		}
	}
	if (vi != ve && *vi == '.')
	{
		fval = ival;
		float dval = 0.1;
		++vi;
		for (; vi != ve; ++vi)
		{
			if (!isDigit( *vi)) break;
			fval += dval * (*vi - '0');
			dval /= 10;
		}
		if (vi == ve)
		{
			return ArithmeticVariant(sign?fval:-fval);
		}
	}
	throw std::runtime_error( std::string( "cannot convert string to arithmetic variant value: '") + valueAsString + "'");
}

static void print( std::ostream& out, const ArithmeticVariant& val)
{
	switch (val.type)
	{
		case ArithmeticVariant::Null: break;
		case ArithmeticVariant::Int: out << val.variant.Int; break;
		case ArithmeticVariant::UInt: out << val.variant.UInt; break;
		case ArithmeticVariant::Float: out << val.variant.Float; break;
	}
}

DLL_PUBLIC std::string strus::arithmeticVariantToString( const ArithmeticVariant& value)
{
	std::ostringstream out;
	print( out, value);
	return out.str();
}

DLL_PUBLIC std::ostream& strus::operator<< (std::ostream& out, const ArithmeticVariant& v)
{
	print( out, v);
	return out;
}



