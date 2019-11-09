/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "uintCompaction.hpp"
#include <cstdlib>
#include <cmath>
#include <stdexcept>

using namespace strus;

struct Uint8CompactionTable
{
	unsigned int ar[ 256];

	Uint8CompactionTable()
	{
		int ai = 0;
		ar[ 0] = 0;
		for (++ai; ai < 256; ++ai)
		{
			ar[ ai] = ar[ ai-1] + (1 + ai/10);
		}
	}
	unsigned int findUpperBound( unsigned int in)
	{
		int as = 0;
		int ae = 255;
		int ai = 3;	///... values more likely to be small

		while (as + 3 < ae)
		{
			if (in <= ar[ ai])
			{
				ae = ai;
			}
			else
			{
				as = ai;
			}
			ai = (as + ae) >> 1;
		}
		for (ai=as; ai<=ae; ++ai)
		{
			if (ar[ai] >= in) return ai;
		}
		return ae;
	}
};

static Uint8CompactionTable g_uint8CompactionTable;

unsigned int strus::uintFromCompaction( uint8_t in)
{
	return g_uint8CompactionTable.ar[ in];
}

uint8_t strus::compactUint( unsigned int in)
{
	return g_uint8CompactionTable.findUpperBound( in);
}

