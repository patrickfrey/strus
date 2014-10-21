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
#include "utils/fileio.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/queryEvalInterface.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <boost/scoped_ptr.hpp>

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <program>" << std::endl;
	std::cerr << "<program> = file with query evaluation program source" << std::endl;
}

int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc > 2)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		std::string prgfile = argv[1];
		std::string prgsource;
		unsigned int ec = strus::readFile( prgfile, prgsource);
		if (ec)
		{
			std::cerr << "ERROR could not read program file '" << prgfile << "', error " << ec << std::endl;
			return 2;
		}
		strus::QueryEvalInterface* qeval = strus::createQueryEval( prgsource);
		std::ostringstream out;
		qeval->print( out);

		std::string result = out.str();
		std::cout << "result:" << std::endl << result << std::endl;
		return 0;
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


