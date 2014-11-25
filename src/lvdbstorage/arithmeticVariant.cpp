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
#include "strus/arithmeticVariant.hpp"
#include "dll_tags.hpp"
#include <sstream>

using namespace strus;

DLL_PUBLIC void ArithmeticVariant::print( std::ostream& out) const
{
	switch (type)
	{
		case Null: break;
		case Int: out << variant.Int; break;
		case UInt: out << variant.UInt; break;
		case Float: out << variant.Float; break;
	}
}

DLL_PUBLIC std::string ArithmeticVariant::tostring() const
{
	std::ostringstream out;
	print( out);
	return out.str();
}

DLL_PUBLIC std::ostream& strus::operator<< (std::ostream& out, const ArithmeticVariant& v)
{
	v.print( out);
	return out;
}

