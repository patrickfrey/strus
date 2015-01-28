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
#include "lvdbstorage/floatConversions.hpp"
#include "lvdbstorage/floatConversions.cpp"
#include "lvdbstorage/indexPacker.hpp"
#include "private/bitOperations.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <cstdlib>

#define RANDINT(MIN,MAX) ((rand()%(MAX+MIN))+MIN)


static void spFloatConversionTest( float in)
{
	float16_t res = strus::floatSingleToHalfPrecision( in);
	float test = strus::floatHalfToSinglePrecision( res);
	float diff = in - test;
	float epsilon = 4.88e-04;
	// half precision float epsilon is 4.88e-04 
	//	according http://en.wikipedia.org/wiki/Machine_epsilon
	if (diff*diff > epsilon)
	{
		throw std::runtime_error("float conversion failed");
	}
	std::cerr << "tested float single/half precision for " << in << " (error " << diff << ")" << std::endl;
}

static void testSinglePrecisionFloatConversions()
{
	spFloatConversionTest( 3.1);
	spFloatConversionTest( 27);
	spFloatConversionTest( 1.9);
	spFloatConversionTest( 2.1);
	spFloatConversionTest( 21.1);
	spFloatConversionTest( -8.1);
	spFloatConversionTest( -0.1);
}

int main( int, const char**)
{
	try
	{
		testSinglePrecisionFloatConversions();
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


