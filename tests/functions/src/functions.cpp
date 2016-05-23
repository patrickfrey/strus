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
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <set>
#include <limits>
#include "strus/base/stdint.h"

#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

using namespace strus;

template <typename SET_UINTTYPE>
struct SetType
{};

template <>
struct SetType<uint32_t>
{
	typedef uint32_t Type;
};

template <>
struct SetType<uint64_t>
{
	typedef uint64_t Type;
};

template <typename SET_UINTTYPE>
static void bitOperationsTest( unsigned int times)
{
	unsigned int nof_bits = 8*sizeof(SET_UINTTYPE);
	for (unsigned int ti=0; ti<times; ++ti)
	{
		std::set<unsigned int> elements;
		unsigned int ee = RANDINT( 0,nof_bits);
		unsigned int ei = 0;
		for (; ei<ee; ++ei)
		{
			unsigned int elem = RANDINT(0,nof_bits);
			elements.insert( elem);
		}
		ee = elements.size();

		std::ostringstream bsetmsg;
		typename SetType<SET_UINTTYPE>::Type bset = 0;
		std::set<unsigned int>::const_iterator si = elements.begin(), se = elements.end();
		int sidx=0;
		for (; si != se; ++si,++sidx)
		{
			typename SetType<SET_UINTTYPE>::Type shf = 1;
			bset |= (shf << *si);
			if (sidx) bsetmsg << " ";
			bsetmsg << *si;
		}
		typename SetType<SET_UINTTYPE>::Type bset_orig = bset;
		for (sidx=0,si=elements.begin(); si != se; ++si,++sidx)
		{
			unsigned int bi = BitOperations::bitScanForward( bset);
			if (!bi) break;
			--bi;
			if (bi != *si)
			{
				char msgbuf[ 512];
				std::string setstr( bsetmsg.str());
				snprintf( msgbuf, sizeof(msgbuf), "bit scan forward %u bits failed: set {%s} [%lu, %lu], read %u, expected %u, index %d", nof_bits, setstr.c_str(), (unsigned long)bset_orig, (unsigned long)bset, bi, *si, sidx);
				throw std::runtime_error( msgbuf);
			}
			typename SetType<SET_UINTTYPE>::Type shf = 1;
			bset &= ~(shf << bi);
		}
	}
}

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
		std::cerr << "executing test SinglePrecisionFloatConversions" << std::endl;
		testSinglePrecisionFloatConversions();
		std::cerr << "executing test BitOperations32" << std::endl;
		bitOperationsTest<uint32_t>( 100);
		std::cerr << "executing test BitOperations64" << std::endl;
		bitOperationsTest<uint64_t>( 100);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


