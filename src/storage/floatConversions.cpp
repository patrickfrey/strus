/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "floatConversions.hpp"
#include <cstdlib>
#include <cmath>
#include <stdexcept>

using namespace strus;

// Solution for the functions in this module 
// from http://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
// Thanks to Martin Källman.

float strus::floatHalfToSinglePrecision( float16_t val)
{
	uint32_t t1;
	uint32_t t2;
	uint32_t t3;
	union
	{
		uint32_t intval;
		float floatval;
	} rt;

	uint16_t in = *(uint16_t*)&val;
	t1 = in & 0x7fff;			// Non-sign bits
	t2 = in & 0x8000;			// Sign bit
	t3 = in & 0x7c00;			// Exponent

	t1 <<= 13;				// Align mantissa on MSB
	t2 <<= 16;				// Shift sign bit into position

	t1 += 0x38000000;			// Adjust bias

	t1 = (t3 == 0 ? 0 : t1);		// Denormals-as-zero

	t1 |= t2;				// Re-insert sign bit

	rt.intval = t1;
	return rt.floatval;
}


float16_t strus::floatSingleToHalfPrecision( float in)
{
	union
	{
		uint32_t intval;
		float floatval;
	} param;
	param.floatval = in;

	uint32_t t1;
	uint32_t t2;
	uint32_t t3;

	t1 = param.intval & 0x7fffffff;		// Non-sign bits
	t2 = param.intval & 0x80000000;		// Sign bit
	t3 = param.intval & 0x7f800000;		// Exponent

	t1 >>= 13;				// Align mantissa on MSB
	t2 >>= 16;				// Shift sign bit into position

	t1 -= 0x1c000;				// Adjust bias

	t1 = (t3 < 0x38800000) ? 0 : t1;	// Flush-to-zero
	t1 = (t3 > 0x8e000000) ? 0x7bff : t1;	// Clamp-to-max
	t1 = (t3 == 0 ? 0 : t1);		// Denormals-as-zero

	t1 |= t2;				// Re-insert sign bit

	float16_t rt = (uint16_t)t1;
	return rt;
}

