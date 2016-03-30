/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

DLL_PUBLIC ErrorBufferInterface* strus::createErrorBuffer_standard( FILE* logfilehandle, std::size_t maxNofThreads_)
{
	try
	{
		return new ErrorBuffer( logfilehandle, maxNofThreads_);
	}
	catch (const std::bad_alloc&)
	{
		fprintf( logfilehandle?logfilehandle:stderr, _TXT("out of memory creating error buffer\n"));
		return 0;
	}
	catch (const std::exception& err)
	{
		fprintf( logfilehandle?logfilehandle:stderr, _TXT("exception creating error buffer: %s\n"), err.what());
		return 0;
	}
}


