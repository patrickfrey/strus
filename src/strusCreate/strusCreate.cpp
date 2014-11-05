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
#include "strus/storageLib.hpp"
#include "strus/queryProcessorLib.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/accumulatorInterface.hpp"
#include "strus/storageInterface.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>

int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		std::cerr << "usage: strusCreate <config>" << std::endl;
		std::cerr << "<config>  : configuration string of the storage";
		std::string indent;
		char const* cc = strus::getStorageConfigDescription();
		char const* ee;
		do
		{
			ee = std::strchr( cc,'\n');
			std::string line = ee?std::string( cc, ee-cc):std::string( cc);
			std::cerr << indent << line << std::endl;
			cc = ee + 1;
			if (indent.empty())
			{
				indent = std::string( 12, ' ');
			}
		}
		while (ee);
		return 0;
	}
	try
	{
		if (argc < 2) throw std::runtime_error( "too few arguments (expected storage configuration string)");
		if (argc > 2) throw std::runtime_error( "too many arguments for strusCreate");

		strus::createStorageDatabase( argv[1]);
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "EXCEPTION " << e.what() << std::endl;
	}
	return -1;
}


