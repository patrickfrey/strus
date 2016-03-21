/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "indexPacker.hpp"
#include "floatConversions.hpp"
#include "private/bitOperations.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <cstdlib>

#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

using namespace strus;

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


