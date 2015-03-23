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
#include "indexPacker.hpp"
#include "private/bitOperations.hpp"
#include <map>
#include <limits>
#include <cmath>
#include <cstring>
#include <stdexcept>

#define B11111111 0xFF
#define B01111111 0x7F
#define B00111111 0x3F
#define B00011111 0x1F
#define B00001111 0x0F
#define B00000111 0x07
#define B00000011 0x03
#define B00000001 0x01
#define B00000000 0x00
#define B10000000 0x80
#define B11000000 0xC0
#define B11100000 0xE0
#define B11110000 0xF0
#define B11111000 0xF8
#define B11111100 0xFC
#define B11111110 0xFE

#define B11011111 (B11000000|B00011111)
#define B11101111 (B11100000|B00001111)
#define B11110111 (B11110000|B00000111)
#define B11111011 (B11111000|B00000011)
#define B11111101 (B11111100|B00000001)

#define RANGE_DELIM ((char)0xFE)

using namespace strus;

static inline unsigned char utf8charlen( unsigned char ch)
{
	unsigned char cl = 9-BitOperations::bitScanReverse( (uint8_t)(ch^0xFF));
	return cl>2?(cl-1):1;
}

static inline int32_t utf8decode( const char* itr, unsigned int charsize)
{
	int32_t res = (unsigned char)*itr;
	if (res > 127)
	{
		res = ((unsigned char)*itr)&(B00011111>>(charsize-2));
		for (unsigned int ii=1; ii<charsize; ii++)
		{
			res <<= 6;
			res |= (unsigned char)(itr[ii] & B00111111);
		}
	}
	return res;
}

static inline std::size_t utf8encode( char* buf, int32_t chr)
{
	if (chr<0)
	{
		throw std::runtime_error( "illegal unicode character");
	}
	if (chr <= 127)
	{
		*buf = (char)(unsigned char)chr;
		return 1;
	}
	uint32_t pp,sf;
	for (pp=1,sf=5; pp<5; pp++,sf+=5)
	{
		if ((uint32_t)chr < (uint32_t)((1<<6)<<sf)) break;
	}
	unsigned char HB = (unsigned char)(B11111111 << (7-pp));
	unsigned char shf = (unsigned char)(pp*6);
	unsigned int ii;
	*buf++ = (char)(((unsigned char)(chr >> shf) & (~HB >> 1)) | HB);
	for (ii=1,shf-=6; ii<=pp; shf-=6,ii++)
	{
		*buf++ = (char)(unsigned char) (((chr >> shf) & B00111111) | B10000000);
	}
	return ii;
}

static inline int32_t unpackInt32_( const char*& itr, const char* end)
{
	int charlen = utf8charlen( *itr);
	if (end - itr < charlen)
	{
		throw std::runtime_error( "corrupt data (unpackInt32_ 1)");
	}
	int32_t rt = utf8decode( itr, charlen);
	if (rt < 0)
	{
		throw std::runtime_error( "corrupt data (unpackInt32_ 2)");
	}
	itr += charlen;
	return rt;
}

const char* strus::nextPackedIndexPos( const char* end, const char* str)
{
	char const* cc = str;
	while (cc < end && ((unsigned char)*cc & B11000000) == B10000000) {++cc;}
	return cc;
}

const char* strus::nextPackedRangePos( const char* end, const char* str)
{
	char const* rt = nextPackedIndexPos( end, str);
	if (rt == end) return rt;
	if (*(rt) == RANGE_DELIM)
	{
		return nextPackedIndexPos( end, rt+1);
	}
	return rt;
}

const char* strus::prevPackedIndexPos( const char* start, const char* str)
{
	char const* cc = str;
	while (((unsigned char)*cc & B11000000) == B10000000) {--cc;}
	if (start >= cc)
	{
		if (start == cc && ((unsigned char)*cc & B11000000) != B10000000) return cc;
		throw std::runtime_error( "corrupt data (nextPackedIndexPos 1)");
	}
	return cc;
}

const char* strus::prevPackedRangePos( const char* start, const char* str)
{
	char const* rt = prevPackedIndexPos( start, str);
	if (rt == start) return rt;
	if (*(rt-1) == RANGE_DELIM)
	{
		return prevPackedIndexPos( start, rt-2);
	}
	return rt;
}

Index strus::unpackIndex( const char*& itr, const char* end)
{
	return (Index)unpackInt32_( itr, end);
}

const char* strus::skipIndex( const char* ptr, const char* end)
{
	char const* rt = ptr;
	if (ptr == end)
	{
		throw std::runtime_error( "corrupt data (skipIndex)");
	}
	rt += utf8charlen( *rt);
	if (rt > end)
	{
		throw std::runtime_error( "corrupt data (skipIndex)");
	}
	return rt;
}

const char* strus::skipRange( const char* ptr, const char* end)
{
	char const* rt = skipIndex( ptr, end);
	if (rt == end) return end;
	if (*rt == RANGE_DELIM)
	{
		return skipIndex( rt+1, end);
	}
	return rt;
}

static inline std::size_t packIndex_( char* buf, const Index& idx)
{
	return utf8encode( buf, (int32_t)idx);
}

static inline std::size_t packRange_( char* buf, const Index& idx, const Index& rangesize)
{
	std::size_t rt = packIndex_( buf, idx);
	if (rangesize)
	{
		buf[ rt++] = RANGE_DELIM;
		rt += packIndex_( buf+rt, rangesize);
	}
	return rt;
}

void strus::packIndex( std::string& buf, const Index& idx)
{
	char buf_[ 16];
	std::size_t size = packIndex_( buf_, idx);
	buf.append( buf_, size);
}

void strus::packIndex( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx)
{
	size += packIndex_( buf+size, idx);
	if (size > maxsize) throw std::logic_error( "array bounds write (packIndex)");
}

void strus::packRange( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx, const Index& rangesize)
{
	size += packRange_( buf, idx, rangesize);
	if (size > maxsize) throw std::logic_error( "array bounds write (packRange)");
}

void strus::packRange( std::string& buf, const Index& idx, const Index& rangesize)
{
	char buf_[ 32];
	std::size_t size = packRange_( buf_, idx, rangesize);
	buf.append( buf_, size);
}

Index strus::unpackRange( const char*& ptr, const char* end, Index& rangesize)
{
	Index rt = unpackIndex( ptr, end);
	if (ptr < end && ptr[0] == RANGE_DELIM)
	{
		++ptr;
		rangesize = unpackIndex( ptr, end);
	}
	else
	{
		rangesize = 0;
	}
	return rt;
}


struct SkipChar
{
	inline bool operator()( char const*& pi, const char* pe, char)
	{
		pi += utf8charlen( *pi);
		return pi!=pe;
	}
};

struct SkipStruct
{
	inline bool operator()( char const*& pi, const char* pe, char delim)
	{
		pi = (char const*)std::memchr( pi, delim, pe-pi);
		if (!pi)
		{
			pi = pe;
			return false;
		}
		else
		{
			++pi;
			return true;
		}
	}
};

struct SkipRange
{
	inline bool operator()( char const*& pi, const char* pe, char delim)
	{
		pi += utf8charlen( *pi);
		if (pi==pe) return false;

		if (*pi == delim)
		{
			++pi;
			pi += utf8charlen( *pi);
			return pi!=pe;
		}
		return true;
	}
};


struct NextChar
{
	inline const char* operator()( char const* pi, const char* pe, char)
	{
		return nextPackedIndexPos( pe, pi);
	}
};

struct NextStruct
{
	inline const char* operator()( char const* pi, const char* pe, char delim)
	{
		const char* ni = (const char*)std::memchr( pi, delim, pe-pi);
		if (!ni)
		{
			return pe;
		}
		else
		{
			return ++ni;
		}
	}
};

struct NextRange
{
	inline const char* operator()( char const* pi, const char* pe, char delim)
	{
		char const* ni = nextPackedIndexPos( pe, pi);
		if (ni != pe && *ni == delim)
		{
			++ni;
			ni += utf8charlen( *ni);
		}
		return ni;
	}
};


struct CompareAsc
{
	inline bool operator()( const unsigned char* ptr, const unsigned char* needle, std::size_t size)
	{
		//return std::memcmp( ptr, needle, size) < 0;
		std::size_t ii = 0;
		for (; ii < size && ptr[ii] == needle[ii]; ++ii){}
		return ii < size && ptr[ii] < needle[ii];
	}
};

struct CompareDesc
{
	inline bool operator()( const unsigned char* ptr, const unsigned char* needle, std::size_t size)
	{
		//return (std::memcmp( ptr, needle, size) > 0);
		std::size_t ii = 0;
		for (; ii < size && ptr[ii] == needle[ii]; ++ii){}
		return ii < size && ptr[ii] > needle[ii];
	}
};

template <class SkipElem, class Comparator, class NextElem, int NextSkipSize>
static const char* findIndex( const char* ptr, const char* end, Index needle, char delim)
{
	char needlebuf[16];
	std::size_t needlesize = utf8encode( needlebuf, needle);

	char const* pi = ptr;
	const char* pe = end;

	while (pi != pe
		&& Comparator()( (const unsigned char*)pi,
				 (const unsigned char*)needlebuf, needlesize)
		&& SkipElem()( pi, pe, delim))
	{
		while (pe - pi > (NextSkipSize*2))
		{
			char const* npi = NextElem()( pi+NextSkipSize, pe, delim);
			if (npi != pe
				&& Comparator()( (const unsigned char*)npi,
						 (const unsigned char*)needlebuf, needlesize))
			{
				pi = npi;
				continue;
			}
			break;
		}
	}
	return (pi == pe)?0:(const char*)pi;
}

const char* strus::findIndexAsc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipChar,CompareAsc,NextChar,20>( ptr, end, needle, 0);
}

const char* strus::findIndexDesc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipChar,CompareDesc,NextChar,20>( ptr, end, needle, 0);
}

const char* strus::findStructIndexAsc( const char* ptr, const char* end, unsigned char delim, const Index& needle)
{
	return findIndex<SkipStruct,CompareAsc,NextStruct,60>( ptr, end, needle, delim);
}
const char* strus::findStructIndexDesc( const char* ptr, const char* end, unsigned char delim, const Index& needle)
{
	return findIndex<SkipStruct,CompareDesc,NextStruct,60>( ptr, end, needle, delim);
}

const char* strus::findRangeIndexAsc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipRange,CompareAsc,NextRange,40>( ptr, end, needle, RANGE_DELIM);
}
const char* strus::findRangeIndexDesc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipRange,CompareDesc,NextRange,40>( ptr, end, needle, RANGE_DELIM);
}

unsigned int strus::nofPackedIndices( const char* ptr, const char* end)
{
	unsigned int rt = 0;
	char const* pi = ptr;
	const char* pe = end;
	while (pi < pe)
	{
		pi += utf8charlen( *pi);
		++rt;
	}
	return rt;
}

bool strus::checkStringUtf8( const char* ptr, std::size_t size)
{
	std::size_t ii = 0;
	std::size_t len = 0;
	for (; ii<size; ii += len)
	{
		len = utf8charlen( ptr[ii]);
		if (ii + len > size || len == 0 || len == 8) return false;
		std::size_t kk = 1;
		for (; kk<len; ++kk)
		{
			if ((ptr[ii+kk] & B11000000) != B10000000) return false;
		}
	}
	return true;
}

void strus::packGlobalCounter( char* buf, std::size_t& size, std::size_t maxsize, const GlobalCounter& cnt)
{
	if (cnt >> 62 != 0) throw std::runtime_error( "counter out of range (packGlobalCounter)");
	Index hi = (Index)(cnt >> 31);
	Index lo = (cnt & (0x7fffFFFFUL));
	packIndex( buf, size, maxsize, hi);
	packIndex( buf, size, maxsize, lo);
}

void strus::packGlobalCounter( std::string& buf, const GlobalCounter& cnt)
{
	if (cnt >> 62 != 0) throw std::runtime_error( "counter out of range (packGlobalCounter)");
	Index hi = (Index)(cnt >> 31);
	Index lo = (cnt & (0x7fffFFFFUL));
	packIndex( buf, hi);
	packIndex( buf, lo);
}

GlobalCounter strus::unpackGlobalCounter( const char*& ptr, const char* end)
{
	Index hi = unpackIndex( ptr, end);
	Index lo = unpackIndex( ptr, end);
	return ((GlobalCounter)hi << 31) | (GlobalCounter)lo;
}

