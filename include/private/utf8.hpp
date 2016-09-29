/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file utf8.hpp
/// \brief Helpers for UTF-8 encoding/decoding
#ifndef _STRUS_STORAGE_UTF8_ENCODING_DECODING_HPP_INCLUDED
#define _STRUS_STORAGE_UTF8_ENCODING_DECODING_HPP_INCLUDED
#include "strus/base/bitOperations.hpp"

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

namespace strus
{

static inline const char* utf8prev( char const* end)
{
	for (--end; ((unsigned char)*end & B11000000) == B10000000; --end){}
	return end;
}

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
		throw strus::runtime_error( _TXT( "illegal unicode character (%s)"), __FUNCTION__);
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

} //namespace
#endif


