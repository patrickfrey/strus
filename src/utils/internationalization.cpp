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
#include "private/dll_tags.hpp"
#include "private/internationalization.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <cstdarg>

#define STRUS_GETTEXT_PACKAGE		"strus-dom"
#define STRUS_GETTEXT_LOCALEDIR		""

std::runtime_error strus::runtime_error( const char* format, ...)
{
	char buffer[ 1024];
	va_list args;
	va_start( args, format);
	const char* formatTranslation = ::dgettext( STRUS_GETTEXT_PACKAGE, format);
	int buffersize = vsnprintf( buffer, sizeof(buffer), formatTranslation, args);
	buffer[ sizeof(buffer)-1] = 0;
	std::runtime_error rt( std::string( buffer, buffersize));
	va_end (args);
	return rt;
}

std::logic_error strus::logic_error( const char* format, ...)
{
	char buffer[ 1024];
	va_list args;
	va_start( args, format);
	const char* formatTranslation = ::dgettext( STRUS_GETTEXT_PACKAGE, format);
	int buffersize = vsnprintf( buffer, sizeof(buffer), formatTranslation, args);
	buffer[ sizeof(buffer)-1] = 0;
	std::logic_error rt( std::string( buffer, buffersize));
	va_end (args);
	return rt;
}

DLL_PUBLIC void strus::initMessageTextDomain()
{
#ifdef ENABLE_NLS
	::bindtextdomain( STRUS_GETTEXT_PACKAGE, STRUS_GETTEXT_LOCALEDIR);
#endif
}


