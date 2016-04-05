/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Helper methods for reading/writing numeric values with platform independent byteorder
/// \file hton.hpp
#ifndef _STRUS_HTON_HPP_INCLUDED
#define _STRUS_HTON_HPP_INCLUDED
#include <stdint.h>
#include <arpa/inet.h>

namespace strus
{

template <typename Scalar>
class ByteOrder
{};

template <>
class ByteOrder<bool>
{
public:
	static bool hton( const bool& value)		{return value;}
	static bool ntoh( const bool& value)		{return value;}
};

template <>
class ByteOrder<uint8_t>
{
public:
	static uint8_t hton( const uint8_t& value)	{return value;}
	static uint8_t ntoh( const uint8_t& value)	{return value;}
};

template <>
class ByteOrder<int8_t>
{
public:
	static int8_t hton( const int8_t& value)	{return value;}
	static int8_t ntoh( const int8_t& value)	{return value;}
};

template <>
class ByteOrder<uint16_t>
{
public:
	static uint16_t hton( const uint16_t& value)	{return htons(value);}
	static uint16_t ntoh( const uint16_t& value)	{return ntohs(value);}
};

template <>
class ByteOrder<int16_t>
{
public:
	static int16_t hton( const int16_t& value)	{return htons(value);}
	static int16_t ntoh( const int16_t& value)	{return ntohs(value);}
};

template <>
class ByteOrder<uint32_t>
{
public:
	static uint32_t hton( const uint32_t& value)	{return htonl(value);}
	static uint32_t ntoh( const uint32_t& value)	{return ntohl(value);}
};

template <>
class ByteOrder<int32_t>
{
public:
	static int32_t hton( const int32_t& value)	{return htonl(value);}
	static int32_t ntoh( const int32_t& value)	{return ntohl(value);}
};

template <>
class ByteOrder<uint64_t>
{
public:
	static uint64_t hton( const uint64_t& value)
	{
		union
		{
			uint32_t p[2];
			uint64_t v;
		} val;
		val.p[0] = htonl( value >> 32);
		val.p[1] = htonl( value & 0xffFFffFF);
		return val.v;
	}
	static uint64_t ntoh( const uint64_t& value)
	{
		union
		{
			uint32_t p[2];
			uint64_t v;
		} val;
		val.p[0] = ntohl( value >> 32);
		val.p[1] = ntohl( value & 0xffFFffFF);
		return val.v;
	}
};

template <>
class ByteOrder<int64_t>
{
public:
	static int64_t hton( const int64_t& value)
	{
		return (int64_t)ByteOrder<uint64_t>::hton((uint64_t)value);
	}
	static int64_t ntoh( const int64_t& value)
	{
		return (int64_t)ByteOrder<uint64_t>::ntoh((uint64_t)value);
	}
};

template <>
class ByteOrder<float>
{
public:
	static float hton( const float& value)
	{
		union
		{
			uint32_t x;
			float v;
		} val;
		val.v = value;
		val.x = ByteOrder<uint32_t>::hton( val.x);
		return val.v;
	}
	static float ntoh( const float& value)
	{
		union
		{
			uint32_t x;
			float v;
		} val;
		val.v = value;
		val.x = ByteOrder<uint32_t>::ntoh( val.x);
		return val.v;
	}
};

template <>
class ByteOrder<double>
{
public:
	static double hton( const double& value)
	{
		union
		{
			uint64_t x;
			float v;
		} val;
		val.v = value;
		val.x = ByteOrder<uint64_t>::hton( val.x);
		return val.v;
	}
	static double ntoh( const double& value)
	{
		union
		{
			uint64_t x;
			float v;
		} val;
		val.v = value;
		val.x = ByteOrder<uint64_t>::ntoh( val.x);
		return val.v;
	}
};

} //namespace
#endif

