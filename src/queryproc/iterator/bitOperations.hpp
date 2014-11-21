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

namespace strus {

struct BitOperations
{
	static inline unsigned int bitScanForward( const uint32_t& idx)
	{
#ifdef __x86_64__
		unsigned int result; 
		if (!idx) return 0;
		asm(" bsf %1, %0 \n" : "=r"(result) : "r"(idx) ); 
		return result+1;
#else
		unsigned int ii = 1;
		uint32_t mask = (1<<31);
		for (;ii<=32 && 0==(idx & mask); ++ii,mask>>=1){}
		return ii;
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
		unsigned int ii = 1;
		uint64_t mask = ((uint64_t)1<<63);
		for (;ii<=32 && 0==(idx & mask); ++ii,mask>>=1){}
		return ii;
#endif
	}
};
}//namespace
#endif
