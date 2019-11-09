/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "uintCompaction.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <limits>
#include <ctime>
#include "strus/base/stdint.h"

static bool g_verbose = false;

static void uintCompactionTest()
{
	for (int ii=0; ii<256; ++ii)
	{
		uint8_t orig_cv = ii;
		unsigned int val = strus::uintFromCompaction( orig_cv);
		if (g_verbose) std::cerr << ii << " -> " << val << std::endl;

		uint8_t cv = strus::compactUint( val);
		if (cv != orig_cv) throw std::runtime_error("test failed");

		if (val > 1)
		{
			cv = strus::compactUint( val-1);
			if (cv != orig_cv && cv != orig_cv-1) throw std::runtime_error("test failed");
		}
		if (val > 2)
		{
			cv = strus::compactUint( val-2);
			if (cv != orig_cv && cv != orig_cv-1 && cv != orig_cv-2) throw std::runtime_error("test failed");
		}
		cv = strus::compactUint( val+1);
		if (cv != orig_cv && cv != orig_cv+1) throw std::runtime_error("test failed");

		cv = strus::compactUint( val+2);
		if (cv != orig_cv && cv != orig_cv+1 && cv != orig_cv+2) throw std::runtime_error("test failed");
	}
}

int main( int argc, const char** argv)
{
	try
	{
		if (argc > 1)
		{
			if (0==std::strcmp( argv[1],"-V"))
			{
				g_verbose = true;
			}
			if (0==std::strcmp( argv[1],"-h"))
			{
				std::cerr << argv[0] << " [-V][-h]" << std::endl;
			}
		}
		uintCompactionTest();

		std::cerr << "OK" << std::endl;
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


