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
#include "dll_tags.hpp"
#include <sstream>
#include <boost/lexical_cast.hpp>

using namespace strus;

DLL_PUBLIC ArithmeticVariant strus::arithmeticVariantFromString( const std::string& valueAsString)
{
	ArithmeticVariant rt;
	if (valueAsString.empty())
	{
		throw std::runtime_error("empty string cannot be converted to arithmetic variant");
	}
	bool sign = (valueAsString[0] == '-');
	bool fraction = (0!=std::strchr( valueAsString.c_str(), '.')
			||0!=std::strchr( valueAsString.c_str(), 'E'));
	const char* typeName;
	try
	{
		if (fraction)
		{
			typeName = "floating point number";
			rt.variant.Float = boost::lexical_cast<float>( valueAsString);
			rt.type = ArithmeticVariant::Float;
		}
		else if (sign)
		{
			typeName = "integer";
			rt.variant.Int = boost::lexical_cast<int>( valueAsString);
			rt.type = ArithmeticVariant::Int;
		}
		else
		{
			typeName = "unsigned integer";
			rt.variant.UInt = boost::lexical_cast<unsigned int>( valueAsString);
			rt.type = ArithmeticVariant::UInt;
		}
		return rt;
	}
	catch (const boost::bad_lexical_cast& err)
	{
		throw std::runtime_error( std::string( "cannot convert string to arithmetic variant value, error when trying to convert to ") + typeName + ": " + err.what());
	}
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


