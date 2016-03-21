/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/private/crc32.hpp"
#include "private/dll_tags.hpp"
#include <stdlib.h>

using namespace strus;
using namespace strus::utils;

class Crc32Table
{
public:
	Crc32Table()
	{
		for (unsigned int ii = 0; ii <= 0xFF; ii++)
		{
			enum {Polynomial=0xEDB88320U};
			uint32_t crc = ii;
			for (unsigned int jj = 0; jj < 8; jj++)
			{
				crc = (crc >> 1) ^ (-int(crc & 1) & Polynomial);
			}
			m_ar[ ii] = crc;
		}
	}
	inline uint32_t operator[]( unsigned char idx) const
	{
		return m_ar[ idx];
	}

private:
	uint32_t m_ar[ 0x100];
};

static const Crc32Table g_crc32Lookup;

///\note CRC32 Standard implementation from blog post http://create.stephan-brumme.com/crc32/#sarwate, Thanks
static uint32_t crc32_standardImplementation_1byte( const void* data, size_t length, uint32_t previousCrc32)
{
	uint32_t crc = ~previousCrc32;
	unsigned char const* current = (unsigned char const*) data;
	while (length--)
	{
		crc = (crc >> 8) ^ g_crc32Lookup[(crc & 0xFF) ^ *current++];
	}
	return ~crc;
}
static uint32_t crc32_standardImplementation_1byte( const char* data, uint32_t previousCrc32)
{
	uint32_t crc = ~previousCrc32;
	unsigned char const* current = (unsigned char const*) data;
	while (*current)
	{
		crc = (crc >> 8) ^ g_crc32Lookup[(crc & 0xFF) ^ *current++];
	}
	return ~crc;
}

DLL_PUBLIC uint32_t Crc32::calc( const char* blk, std::size_t blksize)
{
	return crc32_standardImplementation_1byte( blk, blksize, 0);
}

DLL_PUBLIC uint32_t Crc32::calc( const char* blk)
{
	return crc32_standardImplementation_1byte( blk, 0);
}

