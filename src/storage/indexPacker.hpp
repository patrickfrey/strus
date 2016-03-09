/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
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



