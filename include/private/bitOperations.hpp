/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_BIT_OPERATIONS_HPP_INCLUDED
#define _STRUS_STORAGE_BIT_OPERATIONS_HPP_INCLUDED
#include "strus/index.hpp"
#include <cstdlib>
#include <cstring>

namespace strus {

struct BitOperations
{
	static inline unsigned int bitScanReverse( const uint32_t& idx)
	{
#ifdef __x86_64__
		uint32_t result; 
		if (!idx) return 0;
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		uint32_t xx = idx;
		if (!xx) return 0;
		int ee = 1;
		if ((xx & 0xFFff0000))   { ee +=16; xx >>=16; }
		if ((xx & 0x0000Ff00))   { ee += 8; xx >>= 8; }
		if ((xx & 0x000000F0))   { ee += 4; xx >>= 4; }
		if ((xx & 0x0000000C))   { ee += 2; xx >>= 2; }
		if ((xx & 0x00000002))   { ee += 1; }
		return ee;
#endif
	}

	static inline unsigned int bitScanReverse( const uint16_t& idx)
	{
		uint32_t xx = idx;
		if (!xx) return 0;
#ifdef __x86_64__
		uint32_t result; 
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(xx) ); 
		return result+1;
#else
		int ee = 1;
		if ((xx & 0xFf00))   { ee += 8; xx >>= 8; }
		if ((xx & 0x00F0))   { ee += 4; xx >>= 4; }
		if ((xx & 0x000C))   { ee += 2; xx >>= 2; }
		if ((xx & 0x0002))   { ee += 1; }
		return ee;
#endif
	}

	static inline unsigned int bitScanReverse( const uint8_t& idx)
	{
		uint32_t xx = idx;
		if (!xx) return 0;
#ifdef __x86_64__
		uint32_t result; 
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(xx) ); 
		return result+1;
#else
		int ee = 1;
		if ((xx & 0xF0))   { ee += 4; xx >>= 4; }
		if ((xx & 0x0C))   { ee += 2; xx >>= 2; }
		if ((xx & 0x02))   { ee += 1; }
		return ee;
#endif
	}

	static inline unsigned int bitScanForward( const uint32_t& idx)
	{
#ifdef __x86_64__
		uint32_t result;
		if (!idx) return 0;
		asm(" bsf %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		return ffs( idx);
#endif
	}

	static inline unsigned int bitScanForward( const uint64_t& idx)
	{
#ifdef __x86_64__
		uint64_t result;
		if (!idx) return 0;
		asm(" bsfq %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return (unsigned int)(result+1);
#else
		return ffsl( idx);
#endif
	}

	static inline uint64_t bitInsert( const uint64_t& bitset, unsigned int bi)
	{
		uint64_t mask = ((uint64_t)1<<bi)-1;
		return ((bitset &~ mask) << 1) | ((uint64_t)1<<bi) | (bitset & mask);
	}
};
}//namespace
#endif
