/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "private/dll_tags.hpp"
#include "private/errorUtils.hpp"
#include "strus/private/cmdLineOpt.hpp"
#include "strus/errorBufferInterface.hpp"
#include <iostream>
#include <string>
#include <cstring>

using namespace strus;

DLL_PUBLIC void strus::printIndentMultilineString( std::ostream& out, std::size_t indentsize, const char* descr, ErrorBufferInterface* errorhnd)
{
	try
	{
		std::string indent( indentsize, ' ');
	
		char const* cc = descr;
		char const* ee;
		do
		{
			ee = std::strchr( cc,'\n');
			std::string line = ee?std::string( cc, ee-cc):std::string( cc);
			out << indent << line << std::endl;
			cc = ee + 1;
		}
		while (ee);
	}
	CATCH_ERROR_MAP("error printing multiline string for usage help: %s", *errorhnd);
}


