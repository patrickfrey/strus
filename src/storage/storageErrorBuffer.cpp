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
/// \brief Local implemenation of interface for reporting and catching errors in the core (storage)
/// \file storageErrorBuffer.cpp
#include "storageErrorBuffer.hpp"
#include "strus/private/snprintf.h"
#include <stdarg.h>

using namespace strus;

StorageErrorBuffer::StorageErrorBuffer()
{
	msgbuf[ 0] = '\0';
	hasmsg = false;
}

StorageErrorBuffer::~StorageErrorBuffer(){}

void StorageErrorBuffer::report( const char* format, ...) const
{
	
	if (!hasmsg)
	{
		va_list ap;
		va_start(ap, format);
		strus_vsnprintf( msgbuf, sizeof(msgbuf), format, ap);
		va_end(ap);
		hasmsg = true;
	}
	
}

const char* StorageErrorBuffer::fetchError()
{
	if (!hasmsg) return 0;
	hasmsg = false;
	return msgbuf;
}

bool StorageErrorBuffer::hasError() const
{
	return hasmsg;
}


