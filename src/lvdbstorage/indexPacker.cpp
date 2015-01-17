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

struct CharLengthTab
{
	char m_ar[256];

	unsigned char operator[]( unsigned char ii) const
	{
		return m_ar[ii];
	}

	void fill( unsigned int start, unsigned int end, unsigned char value)
	{
		for (unsigned int ii=start; ii<=end; ++ii) m_ar[ii] = value;
	}

	CharLengthTab()
	{
		fill( B00000000,B01111111,1);
		fill( B11000000,B11011111,2);
		fill( B11100000,B11101111,3);
		fill( B11110000,B11110111,4);
		fill( B11111000,B11111011,5);
		fill( B11111100,B11111101,6);
		fill( B11111110,B11111110,7);
		fill( B11111111,B11111111,8);
	}
};

static CharLengthTab g_charlentable;

static int32_t utf8decode( const char* itr)
{
	unsigned int charsize = g_charlentable[ *itr];
	int32_t res = (unsigned char)*itr;
	if (res > 127)
	{
		unsigned int gg = charsize-2;
		if (gg < 0) return -1;

		res = ((unsigned char)*itr)&(B00011111>>gg);
		for (unsigned int ii=0; ii<=gg; ii++)
		{
			unsigned char xx = (unsigned char)itr[ii+1];
			res = (res<<6) | (xx & B00111111);
			if ((unsigned char)(xx & B11000000) != B10000000)
			{
				return -1;
			}
		}
	}
	return res;
}

template <class BUFFER>
static void utf8encode( BUFFER& buf, int32_t chr)
{
	if (chr<0)
	{
		throw std::runtime_error( "Illegal unicode character");
	}
	uint32_t rt;
	if (chr <= 127)
	{
		buf.push_back( (char)(unsigned char)chr);
		return;
	}
	uint32_t pp,sf;
	for (pp=1,sf=5; pp<5; pp++,sf+=5)
	{
		if ((uint32_t)chr < (uint32_t)((1<<6)<<sf)) break;
	}
	rt = pp+1;
	unsigned char HB = (unsigned char)(B11111111 << (8-rt));
	unsigned char shf = (unsigned char)(pp*6);
	unsigned int ii;
	buf.push_back( (char)(((unsigned char)(chr >> shf) & (~HB >> 1)) | HB));
	for (ii=1,shf-=6; ii<=pp; shf-=6,ii++)
	{
		buf.push_back( (char)(unsigned char) (((chr >> shf) & B00111111) | B10000000));
	}
}

static int32_t unpackInt32_( const char*& itr, const char* end)
{
	int ii;
	int nn = g_charlentable[ *itr];
	char buf[8];
	for (ii=0; itr != end && ii < nn; ++itr,++ii)
	{
		buf[ii] = *itr;
	}
	if (ii < nn || nn == 0)
	{
		throw std::runtime_error( "corrupt data (unpackInt32_ 1)");
	}
	int32_t rt = utf8decode( buf);
	if (rt < 0)
	{
		throw std::runtime_error( "corrupt data (unpackInt32_ 2)");
	}
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
	rt += g_charlentable[ *rt];
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

struct StaticBuffer
{
	StaticBuffer( unsigned char* buf_, std::size_t size_, std::size_t maxsize_)
		:buf(buf_),size(size_),maxsize(maxsize_){}
	StaticBuffer( char* buf_, std::size_t size_, std::size_t maxsize_)
		:buf((unsigned char*)buf_),size(size_),maxsize(maxsize_){}

	void push_back( char chr)
	{
		if (size >= maxsize)
		{
			throw std::runtime_error("static buffer too small");
		}
		buf[ size++] = chr;
	}

	unsigned char* buf;
	std::size_t size;
	std::size_t maxsize;
};

template <class BUFFER>
static void packIndex_( BUFFER& buf, const Index& idx)
{
	if (idx < 0)
	{
		throw std::runtime_error( "index out of range (packIndex_)");
	}
	utf8encode( buf, (int32_t)idx);
}

template <class BUFFER>
static void packRange_( BUFFER& buf, const Index& idx, const Index& rangesize)
{
	packIndex_( buf, idx);
	if (rangesize)
	{
		buf.push_back( RANGE_DELIM);
		packIndex_( buf, rangesize);
	}
}

void strus::packIndex( std::string& buf, const Index& idx)
{
	packIndex_( buf, idx);
}

void strus::packIndex( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx)
{
	StaticBuffer bufstruct( buf, size, maxsize);
	packIndex_( bufstruct, idx);
	size = bufstruct.size;
}

void strus::packRange( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx, const Index& rangesize)
{
	StaticBuffer bufstruct( buf, size, maxsize);
	packRange_( bufstruct, idx, rangesize);
	size = bufstruct.size;
}

void strus::packRange( std::string& buf, const Index& idx, const Index& rangesize)
{
	packRange_( buf, idx, rangesize);
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
	bool operator()( char const*& pi, const char* pe, char)
	{
		pi += g_charlentable[ *pi];
		return pi!=pe;
	}
};

struct SkipStruct
{
	bool operator()( char const*& pi, const char* pe, char delim)
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
	bool operator()( char const*& pi, const char* pe, char delim)
	{
		pi += g_charlentable[ *pi];
		if (pi==pe) return false;

		if (*pi == delim)
		{
			++pi;
			pi += g_charlentable[ *pi];
			return pi!=pe;
		}
		return true;
	}
};

struct CompareAsc
{
	bool operator()( const unsigned char* ptr, const unsigned char* needle, std::size_t size)
	{
		//return std::memcmp( ptr, needle, size) < 0;
		std::size_t ii = 0;
		for (; ii < size && ptr[ii] == needle[ii]; ++ii){}
		return ii < size && ptr[ii] < needle[ii];
	}
};

struct CompareDesc
{
	bool operator()( const unsigned char* ptr, const unsigned char* needle, std::size_t size)
	{
		//return (std::memcmp( ptr, needle, size) > 0);
		std::size_t ii = 0;
		for (; ii < size && ptr[ii] == needle[ii]; ++ii){}
		return ii < size && ptr[ii] > needle[ii];
	}
};

template <class SkipElem, class Comparator>
static const char* findIndex( const char* ptr, const char* end, Index needle, char delim)
{
	unsigned char needlebuf[16];
	StaticBuffer bufstruct( needlebuf, 0, sizeof(needlebuf));
	utf8encode( bufstruct, needle);

	char const* pi = ptr;
	const char* pe = end;

	while (pi != pe
		&& Comparator()( (const unsigned char*)pi, needlebuf, bufstruct.size)
		&& SkipElem()( pi, pe, delim)){}

	return (pi == pe)?0:(const char*)pi;
}

const char* strus::findIndexAsc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipChar,CompareAsc>( ptr, end, needle, 0);
}

const char* strus::findIndexDesc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipChar,CompareDesc>( ptr, end, needle, 0);
}

const char* strus::findStructIndexAsc( const char* ptr, const char* end, unsigned char delim, const Index& needle)
{
	return findIndex<SkipStruct,CompareAsc>( ptr, end, needle, delim);
}
const char* strus::findStructIndexDesc( const char* ptr, const char* end, unsigned char delim, const Index& needle)
{
	return findIndex<SkipStruct,CompareDesc>( ptr, end, needle, delim);
}

const char* strus::findRangeIndexAsc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipRange,CompareAsc>( ptr, end, needle, RANGE_DELIM);
}
const char* strus::findRangeIndexDesc( const char* ptr, const char* end, const Index& needle)
{
	return findIndex<SkipRange,CompareDesc>( ptr, end, needle, RANGE_DELIM);
}

unsigned int strus::nofPackedIndices( const char* ptr, const char* end)
{
	unsigned int rt = 0;
	char const* pi = ptr;
	const char* pe = end;
	while (pi < pe)
	{
		pi += g_charlentable[ *pi];
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
		len = g_charlentable[ ptr[ii]];
		if (ii + len > size || len == 0 || len == 8) return false;
		std::size_t kk = 1;
		for (; kk<len; ++kk)
		{
			if ((ptr[ii+kk] & B11000000) != B10000000) return false;
		}
	}
	return true;
}

