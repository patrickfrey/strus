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
/// \brief Exported functions of the strus standard error buffer library
/// \file libstrus_error.cpp
#include "strus/lib/error.hpp"
#include "private/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "errorBuffer.hpp"
#include <stdexcept>
#include <cstdio>

using namespace strus;

DLL_PUBLIC ErrorBufferInterface* strus::createErrorBuffer_standard( FILE* logfilehandle)
{
	try
	{
		return new ErrorBuffer( logfilehandle);
	}
	catch (const std::bad_alloc&)
	{
		fprintf( logfilehandle?logfilehandle:stderr, _TXT("out of memory creating error buffer"));
		return 0;
	}
	catch (const std::exception& err)
	{
		fprintf( logfilehandle?logfilehandle:stderr, _TXT("exception creating error buffer: %s"), err.what());
		return 0;
	}
}


