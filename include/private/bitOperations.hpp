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
	static inline unsigned int bitCount( uint32_t v)
	{
#ifdef __GNUC__
		return __builtin_popcount( v);
#else
		// Taken from 'http://graphics.stanford.edu/~seander/bithacks.html':
		v = v - ((v >> 1) & 0x55555555);			// reuse input as temporary
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333);		// temp
		return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;	// count
#endif
	}

	static inline unsigned int bitCount( uint64_t x)
	{
#ifdef __GNUC__
		return __builtin_popcountll( x);
#else
		// Taken from 'https://en.wikipedia.org/wiki/Hamming_weight':
		const uint64_t m1  = 0x5555555555555555; //binary: 0101...
		const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
		const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
		const uint64_t m8  = 0x00ff00ff00ff00ff; //binary:  8 zeros,  8 ones ...
		const uint64_t m16 = 0x0000ffff0000ffff; //binary: 16 zeros, 16 ones ...
		const uint64_t m32 = 0x00000000ffffffff; //binary: 32 zeros, 32 ones
		const uint64_t hff = 0xffffffffffffffff; //binary: all ones
		const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...

		x -= (x >> 1) & m1;		//put count of each 2 bits into those 2 bits
		x = (x & m2) + ((x >> 2) & m2);	//put count of each 4 bits into those 4 bits 
		x = (x + (x >> 4)) & m4;	//put count of each 8 bits into those 8 bits 
		return (x * h01)>>56;		//returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
#endif
	}

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
