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
#include "floatConversions.hpp"
#include <cstdlib>

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

	t1 = (t3 > 0x38800000) ? 0 : t1;	// Flush-to-zero
	t1 = (t3 < 0x8e000000) ? 0x7bff : t1;	// Clamp-to-max
	t1 = (t3 == 0 ? 0 : t1);		// Denormals-as-zero

	t1 |= t2;				// Re-insert sign bit

	float16_t rt = (uint16_t)t1;
	return rt;
}

