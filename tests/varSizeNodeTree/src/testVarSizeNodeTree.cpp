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
#include "varSizeNodeTree.cpp"
#include "private/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <iomanip>

#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

static std::string doubleToString( double val_)
{
	unsigned int val = ::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val & 1000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(3) << val_ms;
	return val_str.str();
}

static std::string randomKey( unsigned int maxlen)
{
	enum {AlphabetSize=71};
	static const char alphabet[AlphabetSize+1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,?()/%&#";
	std::string rt;
	unsigned int limit = maxlen-1;
	while (RANDINT(1,maxlen) < limit && RANDINT(0,100) >= 5)
	{
		limit -= 1;
		rt.push_back( alphabet[ RANDINT(0,AlphabetSize)]);
	}
	return rt;
}

int main( int argc, const char** argv)
{
	try
	{
		if (argc < 3)
		{
			throw std::runtime_error( "missing arguments <nof inserts> <nof queries>");
		}
		unsigned int nofInserts;
		unsigned int nofQueries;
		try
		{
			nofInserts = utils::toint( argv[1]);
			nofQueries = utils::toint( argv[2]);
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error( std::string("bad values for arguments <nof inserts> <nof queries> (2 non negative integers expected): ") + e.what());
		}
		typedef std::map<std::string,strus::VarSizeNodeTree::NodeData> TestMap;
		TestMap testmap;
		strus::VarSizeNodeTree origmap;
		std::vector<std::string> keyar;

		std::size_t ii = 0;
		for (; ii<nofInserts; ++ii)
		{
			std::string key;
			do
			{
				key = randomKey( 32);
			}
			while (testmap.find(key) != testmap.end());

			testmap[ key] = ii+1;
			keyar.push_back( key);
		}
		testmap.clear();

		std::clock_t start;
		double duration;
		start = std::clock();

		for (ii=0; ii<nofInserts; ++ii)
		{
			testmap[ keyar[ ii]] = ii+1;
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "inserted " << nofInserts << " keys in STL map in " << doubleToString(duration) << " seconds" << std::endl;

		start = std::clock();
		for (ii=0; ii<nofInserts; ++ii)
		{
			origmap.set( keyar[ ii].c_str(), ii+1);
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "inserted " << nofInserts << " keys in variable size node tree in " << doubleToString(duration) << " seconds" << std::endl;

		start = std::clock();
		for (ii=0; ii<nofQueries; ++ii)
		{
			unsigned int keyidx = RANDINT(0,nofInserts);
			TestMap::const_iterator
				mi = testmap.find( keyar[ keyidx]);
			if (mi == testmap.end() || mi->second != keyidx+1)
			{
				throw std::logic_error("TESTMAP LOOKUP FAILED");
			}
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "queried STL map with " << nofQueries << " random selected keys in " << doubleToString(duration) << " seconds" << std::endl;

		start = std::clock();
		for (ii=0; ii<nofQueries; ++ii)
		{
			unsigned int keyidx = RANDINT(0,nofInserts);
			strus::VarSizeNodeTree::NodeData val;
			if (!origmap.find( keyar[ keyidx].c_str(), val)
			||  val != keyidx+1)
			{
				throw std::runtime_error( "VARIABLE SIZE NODE LOOKUP FAILED");
			}
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "queried variable size node tree " << nofQueries << " random selected keys in " << doubleToString(duration) << " seconds" << std::endl;

		TestMap::const_iterator
			ti = testmap.begin(), te = testmap.end();

		for (; ti != te; ++ti)
		{
			strus::VarSizeNodeTree::NodeData val;
			if (!origmap.find( ti->first.c_str(), val))
			{
				throw std::runtime_error( std::string( "inserted key '") + ti->first + "' disapeared in variable size node tree");
			}
		}

		strus::VarSizeNodeTree::const_iterator
			oi = origmap.begin(), oe = origmap.end();
		std::size_t oidx = 0;
		for (; oi != oe; ++oi,++oidx)
		{
			TestMap::const_iterator ti = testmap.find( oi.key());
			if (ti == testmap.end())
			{
				throw std::runtime_error( std::string( "non existing key '") + oi.key() + "' found in variable size node tree");
			}
			else if (ti->second != oi.data())
			{
				std::ostringstream dt;
				dt << ti->second << " != " << oi.data();
				throw std::runtime_error( std::string( "inserted key '") + oi.key() + "' has not the expected value (" + dt.str() + ")");
			}
		}
		if (oidx != nofInserts)
		{
			std::ostringstream dt;
			dt << oidx << " != " << nofInserts;
			throw std::runtime_error( std::string( "number of inserts in variable size node tree does not match (") + dt.str() + ")");
		}
		std::cerr << "checked inserted content in maps. OK" << std::endl;
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


