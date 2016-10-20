/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include "strus/base/utf8.hpp"
#include "strus/base/bitOperations.hpp"
#include <map>
#include <limits>
#include <cmath>
#include <cstring>
#include <stdexcept>

#define RANGE_DELIM ((char)0xFE)

using namespace strus;

static inline int32_t unpackInt32_( const char*& itr, const char* end)
{
	int charlen = utf8charlen( *itr);
	if (end - itr < charlen)
	{
		throw strus::runtime_error( _TXT( "corrupt data (%s 1)"), __FUNCTION__);
	}
	int32_t rt = utf8decode( itr, charlen);
	if (rt < 0)
	{
		throw strus::runtime_error( _TXT( "corrupt data (%s 2)"), __FUNCTION__);
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
		throw strus::runtime_error( _TXT( "corrupt data (%s 1)"), __FUNCTION__);
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
		throw strus::runtime_error( _TXT( "corrupt data (%s)"), __FUNCTION__);
	}
	rt += utf8charlen( *rt);
	if (rt > end)
	{
		throw strus::runtime_error( _TXT( "corrupt data (%s)"), __FUNCTION__);
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
	if (size > maxsize) throw strus::logic_error( _TXT( "array bounds write (%s)"), __FUNCTION__);
}

void strus::packRange( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx, const Index& rangesize)
{
	size += packRange_( buf, idx, rangesize);
	if (size > maxsize) throw strus::logic_error( _TXT( "array bounds write (%s)"), __FUNCTION__);
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
	if (cnt >> 62 != 0) throw strus::runtime_error( _TXT( "counter out of range (%s)"), __FUNCTION__);
	Index hi = (Index)(cnt >> 31);
	Index lo = (cnt & (0x7fffFFFFUL));
	packIndex( buf, size, maxsize, hi);
	packIndex( buf, size, maxsize, lo);
}

void strus::packGlobalCounter( std::string& buf, const GlobalCounter& cnt)
{
	if (cnt >> 62 != 0) throw strus::runtime_error( _TXT( "counter out of range (%s)"), __FUNCTION__);
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

