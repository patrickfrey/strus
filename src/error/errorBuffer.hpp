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
/// \brief Standard implemenation of interface for reporting and catching errors in the core (storage)
/// \file errorBuffer.hpp
#ifndef _STRUS_ERROR_BUFFER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_ERROR_BUFFER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/errorBufferInterface.hpp"
#include <cstdio>

/// \brief strus toplevel namespace
namespace strus
{

/// \class StorageErrorBuffer
class ErrorBuffer
	:public ErrorBufferInterface
{
public:
	explicit ErrorBuffer( FILE* logfilehandle_);
	virtual ~ErrorBuffer();

	virtual void report( const char* format, ...) const;

	virtual void explain( const char* format) const;

	virtual const char* fetchError();

	virtual bool hasError() const;

private:
	enum {MsgBufSize=512};
	mutable char msgbuf[ MsgBufSize];
	mutable bool hasmsg;
	mutable FILE* logfilehandle;
};

}//namespace
#endif

