/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus scalarfunc library
/// \file libstrus_scalarfunc.cpp
#include "strus/lib/scalarfunc.hpp"
#include "strus/errorBufferInterface.hpp"
#include "scalarFunction.hpp"
#include "scalarFunctionInstance.hpp"
#include "scalarFunctionParser.hpp"
#include "private/dll_tags.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"

using namespace strus;

DLL_PUBLIC ScalarFunctionParserInterface* strus::createScalarFunctionParser_default( ErrorBufferInterface* errorhnd)
{
	try
	{
		static bool intl_initialized = false;
		if (!intl_initialized)
		{
			strus::initMessageTextDomain();
			intl_initialized = true;
		}
		return new ScalarFunctionParser( errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating default scalar function parser: %s"), *errorhnd, 0);
}



