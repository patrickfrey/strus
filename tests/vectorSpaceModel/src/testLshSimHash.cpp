/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Test program for the similarity Hash data structure
#include "simHash.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <stdexcept>

#undef STRUS_LOWLEVEL_DEBUG

static void initRandomNumberGenerator()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	unsigned int seed = (now->tm_year+10000) + (now->tm_mon+100) + (now->tm_mday+1);
	std::srand( seed+2);
}

static strus::SimHash createDivBitSet( unsigned int size, unsigned int div)
{
	strus::SimHash rt( size, false);
	unsigned int ki=2, ke=size/2;
	for (; ki<=ke; ++ki)
	{
		if ((div*ki) <= size)
		{
			rt.set( (div*ki)-1, true);
		}
	}
	return rt;
}

static bool isPrimeNumber( unsigned int num)
{
	unsigned int di=2, de=(unsigned int)(std::sqrt(num));
	for (; di <= de; ++di)
	{
		if (num % di == 0) return false;
	}
	return true;
}

static strus::SimHash createPrimBitSet( unsigned int size)
{
	strus::SimHash rt( size, false);
	unsigned int ii=2;
	for (; ii<=size; ++ii)
	{
		if (isPrimeNumber( ii)) 
		{
			rt.set( ii-1, true);
		}
	}
	return rt;
}

static void doMatch( const char* text, const strus::SimHash& res, const strus::SimHash& exp)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cout << "checking " << text << " of " << res.tostring() << std::endl;
#endif
	if (exp.tostring() != res.tostring())
	{
		std::cerr << "RES " << res.tostring() << std::endl;
		std::cerr << "EXP " << exp.tostring() << std::endl;
		throw std::runtime_error( std::string("matching of '") + text + "' failed");
	}
}

int main( int argc, const char** argv)
{
	try
	{
		enum {NofTests=100,MaxSize=1000};
		initRandomNumberGenerator();
		unsigned int sizear[ NofTests];
		sizear[0] = 32;
		sizear[1] = 64;
		sizear[2] = 128;
		sizear[3] = 31;
		sizear[4] = 63;
		sizear[5] = 127;
		sizear[6] = 33;
		sizear[7] = 65;
		sizear[8] = 129;
		unsigned int ti=9, te=NofTests;
		for (; ti != te; ++ti)
		{
			sizear[ti] = rand() % MaxSize;
			
		}
		for (ti=0; ti != te; ++ti)
		{
			std::cerr << "test " << (ti+1) << " size " << sizear[ti] << std::endl;
			strus::SimHash expected = createPrimBitSet( sizear[ti]);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "expected " << expected.tostring() << std::endl;
#endif
			strus::SimHash res = createDivBitSet( sizear[ti], 2);
			res.set( 0, true);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "result   " << res.tostring() << std::endl;
#endif
			unsigned int di=3, de=sizear[ti]/2;
			for (; di < de; ++di)
			{
				res |= createDivBitSet( sizear[ti], di);
			}
			res = ~res;

			strus::SimHash zerofill( sizear[ti], false);
			doMatch( " primes inverse divisible", res, expected);
			doMatch( " result AND expected equals OR", res & expected, res | expected);
			doMatch( " result XOR expected equals reset", res ^ expected, zerofill);

			strus::SimHash res_AND( res); res_AND &= expected;
			strus::SimHash res_OR( res); res_OR |= expected;
			doMatch( " result AND ASSIGN expected equals OR ASSIGN", res_AND, res_OR);
			strus::SimHash res_XOR( res); res_XOR ^= expected;
			doMatch( " result AND ASSIGN expected equals OR ASSIGN", res_XOR, zerofill);
		}
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -1;
	}
	catch (const std::bad_alloc& )
	{
		std::cerr << "out of memory" << std::endl;
		return -2;
	}
	catch (const std::logic_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -3;
	}
}

