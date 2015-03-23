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
#ifndef _STRUS_LVDB_BIT_OPERATIONS_HPP_INCLUDED
#define _STRUS_LVDB_BIT_OPERATIONS_HPP_INCLUDED
#include "strus/index.hpp"
#include <cstdlib>
#include <cstring>

namespace strus {

struct BitOperations
{
	static inline unsigned int bitScanReverse( const uint32_t& idx)
	{
#ifdef __x86_64__
		unsigned int result; 
		if (!idx) return 0;
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		unsigned int xx = idx;
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
#ifdef __x86_64__
		unsigned int result; 
		if (!idx) return 0;
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		unsigned int xx = idx;
		if (!xx) return 0;
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
#ifdef __x86_64__
		unsigned int result; 
		if (!idx) return 0;
		asm(" bsr %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		unsigned int xx = idx;
		if (!xx) return 0;
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
		unsigned int result; 
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
};
}//namespace
#endif
