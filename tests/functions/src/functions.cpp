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
#include "lvdbstorage/indexPacker.hpp"
#include "lvdbstorage/indexPacker.cpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

void testFloatPacking( unsigned int times)
{
	unsigned int tt=0;
	for (; tt<times; ++tt)
	{
		float rnd = static_cast <float> (rand() % (1 << (rand() % 18)))
				+ (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
		std::cerr << "test packing of '" << rnd << "'" << std::endl;
		std::string buf;
		strus::packFloat( buf, rnd);
		const char* itr = buf.c_str();
		const char* end = buf.c_str() + buf.size();
		float res = strus::unpackFloat( itr, end);
		if (res != rnd)
		{
			throw std::runtime_error( "unpacked float result not equal");
		}
		if (end != itr)
		{
			throw std::runtime_error( "iterator after unpack float not equal");
		}
	}
	std::cerr << "tested float packing " << times << " times with success" << std::endl;
}

int main( int, const char**)
{
	try
	{
		testFloatPacking( 100);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


