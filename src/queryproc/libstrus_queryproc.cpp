/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/lib/queryproc.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "queryProcessor.hpp"
#include "strus/base/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

DLL_PUBLIC QueryProcessorInterface* strus::createQueryProcessor( const FileLocatorInterface* filelocator, ErrorBufferInterface* errorhnd)
{
	static bool intl_initialized = false;
	try
	{
		if (!intl_initialized)
		{
			strus::initMessageTextDomain();
			intl_initialized = true;
		}
		return new QueryProcessor( filelocator, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating query processor: %s"), *errorhnd, 0);
}


