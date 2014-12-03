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
#include "varSizeNodeTree.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#define RANDINT(MIN,MAX) ((rand()%(MAX+MIN))+MIN)

static std::string randomKey( unsigned int maxlen)
{
	enum {AlphabetSize=71};
	static const char alphabet[AlphabetSize+1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,?()/%&#";
	std::string rt;
	unsigned int limit = maxlen-1;
	while (RANDINT(1,maxlen) < limit && RANDINT(0,10) > 2)
	{
		limit -= 1;
		rt.push_back( alphabet[ RANDINT(0,AlphabetSize)]);
	}
	return rt;
}

int main( int, const char**)
{
	try
	{
		std::map<std::string,VarSizeNodeTree::Value> testmap;
		VarSizeNodeTree origmap;

		std::size_t ii=0, nofInserts=10000;
		for (; ii<nofInserts; ++ii)
		{
			std::string key = randomKey( 32);
			origmap.set( key.c_str(), ii+1);
			testmap[ key] = ii+1;
		}
		
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


