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
#include "invTermBlock.hpp"
#include "indexPacker.hpp"

using namespace strus;

InvTermBlock::Element InvTermBlock::element_at( const char* itr) const
{
	InvTermBlock::Element rt;
	if (itr == charend()) return rt;
	char const* ri = itr;
	rt.typeno = unpackIndex( ri, charend());
	rt.termno = unpackIndex( ri, charend());
	rt.df = unpackIndex( ri, charend());
	return rt;
}

const char* InvTermBlock::next( const char* ref) const
{
	if (ref == charend()) return 0;
	char const* ri = ref;
	ri = skipIndex( ri, charend());
	ri = skipIndex( ri, charend());
	ri = skipIndex( ri, charend());
	return ri;
}

void InvTermBlock::append( const Index& typeno, const Index& termno, const Index& df)
{
	std::string elem;
	packIndex( elem, typeno);
	packIndex( elem, termno);
	packIndex( elem, df);
	DataBlock::append( elem.c_str(), elem.size());
}

