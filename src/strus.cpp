/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "queryeval/queryEval.hpp"
#include "queryeval/queryProcessor.hpp"
#include <iostream>
#include <cstring>

enum Command {None,CreateStorage};

int main( int argc, const char* argv[])
{
	const char* name = 0;
	const char* cfg = 0;
	Command cmd = None;

	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		std::cerr << "usage: strus <cmd> ..." << std::endl;
		std::cerr << "<cmd> = create  :create a storage" << std::endl;
		return 0;
	}
	try
	{
		if (std::strcmp( argv[1], "create") == 0)
		{
			if (argc <= 3) throw std::runtime_error( "too few argumens for create. (strus create <repository name> [<repository config>])");
			if (argc > 4) throw std::runtime_error( "too many argumens for create. (strus create <repository name> [<repository config>])");

			cmd = CreateStorage;
			name = argv[2];
			cfg = (argc>=4)?argv[3]:0;
		}
		else
		{
			throw std::runtime_error( std::string( "unknown command '") + argv[1] + "' (expected create,..)" );
		}
		switch (cmd)
		{
			case None:
			break;
			case CreateStorage:
				strus::createStorage( name, cfg);
			break;
		}
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


