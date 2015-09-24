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
/// \brief Implemenation of interface for reporting and catching errors in strus for implementing an expcetion free interface
/// \file errorBuffer.cpp
#include "errorBuffer.hpp"
#include "strus/private/snprintf.h"
#include <stdarg.h>
#include <cstring>

using namespace strus;

ErrorBuffer::ErrorBuffer( FILE* logfilehandle_)
	:logfilehandle(logfilehandle_)
{
	msgbuf[ 0] = '\0';
	hasmsg = false;
}

ErrorBuffer::~ErrorBuffer(){}

void ErrorBuffer::report( const char* format, ...) const
{
	
	if (!hasmsg)
	{
		va_list ap;
		va_start(ap, format);
		strus_vsnprintf( msgbuf, sizeof(msgbuf), format, ap);
		if (logfilehandle)
		{
			vfprintf( logfilehandle, format, ap);
			fputs( "\n", logfilehandle);
		}
		va_end(ap);
		hasmsg = true;
	}
	else if (logfilehandle)
	{
		va_list ap;
		va_start(ap, format);
		if (logfilehandle)
		{
			vfprintf( logfilehandle, format, ap);
			fputs( "\n", logfilehandle);
		}
		va_end(ap);
	}
}

void ErrorBuffer::explain( const char* format) const
{
	char newmsgbuf[ MsgBufSize];
	strus_snprintf( newmsgbuf, sizeof(newmsgbuf), format, msgbuf);
	if (logfilehandle)
	{
		fprintf( logfilehandle, format, msgbuf);
		fputs( "\n", logfilehandle);
	}
	std::strcpy( msgbuf, newmsgbuf);
	hasmsg = true;
}

const char* ErrorBuffer::fetchError()
{
	if (!hasmsg) return 0;
	hasmsg = false;
	return msgbuf;
}

bool ErrorBuffer::hasError() const
{
	return hasmsg;
}


