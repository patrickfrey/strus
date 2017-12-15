/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "compactNodeTrie.hpp"
#include "strus/index.hpp"
#include "strus/base/numstring.hpp"
#include "private/stringMap.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <limits>

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}
#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

static std::string doubleToString( double val_)
{
	unsigned int val = (unsigned int)::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val % 1000;
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
			nofInserts = strus::numstring_conv::touint( argv[ 1], std::numeric_limits<int>::max());
			nofQueries = strus::numstring_conv::touint( argv[ 2], std::numeric_limits<int>::max());
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error( std::string("bad values for arguments <nof inserts> <nof queries> (2 non negative integers expected): ") + e.what());
		}
		initRand();
		typedef strus::StringMap<strus::Index> TestMap;
		TestMap testmap;
		conotrie::CompactNodeTrie origmap;
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
			if (mi == testmap.end() || mi->second != (strus::Index)keyidx+1)
			{
				throw std::logic_error("TESTMAP LOOKUP FAILED");
			}
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "queried boost unordered map with " << nofQueries << " random selected keys in " << doubleToString(duration) << " seconds" << std::endl;

		start = std::clock();
		for (ii=0; ii<nofQueries; ++ii)
		{
			unsigned int keyidx = RANDINT(0,nofInserts);
			conotrie::CompactNodeTrie::NodeData val;
			if (!origmap.get( keyar[ keyidx].c_str(), val)
			||  val != keyidx+1)
			{
				throw std::runtime_error( "VARIABLE SIZE NODE LOOKUP FAILED");
			}
		}
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "queried variable size node tree " << nofQueries << " random selected keys in " << doubleToString(duration) << " seconds" << std::endl;
		{
			TestMap::const_iterator
				ti = testmap.begin(), te = testmap.end();
	
			for (; ti != te; ++ti)
			{
				conotrie::CompactNodeTrie::NodeData val;
				if (!origmap.get( ti->first, val))
				{
					throw std::runtime_error( std::string( "inserted key '") + ti->first + "' disapeared in variable size node tree");
				}
			}
		}
		conotrie::CompactNodeTrie::const_iterator
			oi = origmap.begin(), oe = origmap.end();
		std::size_t oidx = 0;
		for (; oi != oe; ++oi,++oidx)
		{
			TestMap::const_iterator ti = testmap.find( oi.key());
			if (ti == testmap.end())
			{
				throw std::runtime_error( std::string( "non existing key '") + oi.key() + "' found in variable size node tree");
			}
			else if (ti->second != (strus::Index)oi.data())
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


