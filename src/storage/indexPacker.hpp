/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_STORAGE_ENCODE_HPP_INCLUDED
#define _STRUS_STORAGE_STORAGE_ENCODE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>

namespace strus
{
void packIndex( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx);
void packIndex( std::string& buf, const Index& idx);
Index unpackIndex( const char*& ptr, const char* end);
void packGlobalCounter( char* buf, std::size_t& size, std::size_t maxsize, const GlobalCounter& cnt);
void packGlobalCounter( std::string& buf, const GlobalCounter& cnt);
GlobalCounter unpackGlobalCounter( const char*& ptr, const char* end);
const char* skipIndex( const char* ptr, const char* end);

void packRange( char* buf, std::size_t& size, std::size_t maxsize, const Index& idx, const Index& rangesize);
void packRange( std::string& buf, const Index& idx, const Index& rangesize);
Index unpackRange( const char*& ptr, const char* end, Index& rangesize);
const char* skipRange( const char* ptr, const char* end);

const char* findIndexAsc( const char* ptr, const char* end, const Index& needle);
const char* findIndexDesc( const char* ptr, const char* end, const Index& needle);
const char* findStructIndexAsc( const char* ptr, const char* end, unsigned char delim, const Index& needle);
const char* findStructIndexDesc( const char* ptr, const char* end, unsigned char delim, const Index& needle);
const char* findRangeIndexAsc( const char* ptr, const char* end, const Index& needle);
const char* findRangeIndexDesc( const char* ptr, const char* end, const Index& needle);

unsigned int nofPackedIndices( const char* ptr, const char* end);
const char* nextPackedIndexPos( const char* end, const char* str);
const char* nextPackedRangePos( const char* end, const char* str);
const char* prevPackedIndexPos( const char* start, const char* str);
const char* prevPackedRangePos( const char* start, const char* str);

bool checkStringUtf8( const char* ptr, std::size_t size);

}//namespace
#endif



