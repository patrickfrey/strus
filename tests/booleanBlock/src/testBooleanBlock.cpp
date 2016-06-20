/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/index.hpp"
#include "indexPacker.hpp"
#include "dataBlock.hpp"
#include "booleanBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <set>

#undef STRUS_LOWLEVEL_DEBUG

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}
#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)


static void testBooleanBlock( unsigned int times, unsigned int minNofElements, unsigned int nofQueries)
{
	unsigned int tt=0;
	
	for (; tt<times; ++tt)
	{
		strus::Index maxElemNo = 0;
		strus::Index elemItr = 0;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "test " << tt << ":" << std::endl;
#endif
		std::set<strus::Index> indexSet;
		strus::BooleanBlock blk;

		unsigned int ii=0,nofElements=minNofElements+RANDINT(1,100);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "ELEMENTS:";
#endif
		for (; ii<nofElements; ++ii)
		{
			elemItr += RANDINT(1,25);
			strus::Index range = RANDINT(1,25);
			strus::Index ri = 0, re = range;
			for (; ri <= re; ++ri)
			{
				indexSet.insert( elemItr + ri);
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << " " << elemItr << ":" << (elemItr+range);
#endif
			blk.defineRange( elemItr, range);
			maxElemNo = elemItr + range;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << std::endl;
		std::cout << "RANGES:";
		strus::BooleanBlock::NodeCursor ci;
		strus::Index from_, to_;
		bool more = blk.getFirstRange( ci, from_, to_);
		for (; more; more = blk.getNextRange( ci, from_, to_))
		{
			std::cout << " " << from_ << ":" << to_;
		}
		std::cout << std::endl;
#endif
		strus::BooleanBlock::NodeCursor cursor;
		strus::Index positiveCnt = 0;
		for (ii=0; ii<nofQueries; ++ii)
		{
			strus::Index match_i = RANDINT( 2, maxElemNo+1);
			strus::Index prev_i = match_i;
			std::set<strus::Index>::const_iterator si = indexSet.find( match_i);
			bool expected = si != indexSet.end();
			if (expected)
			{
				si = indexSet.upper_bound( match_i-2);
				if (*si == match_i)
				{
					prev_i = match_i-1;
				}
			}
			bool output = match_i == blk.skip( prev_i, cursor);

#ifdef STRUS_LOWLEVEL_DEBUG
			std::cout << "\tQUERY " << prev_i << " => " << match_i << ": "
					<< (expected?"expected":"not expected") << ", "
					<< (output?"in":"not in") << std::endl;
#endif
			if (expected)
			{
				++positiveCnt;
			}
			if (output != expected)
			{
				std::ostringstream item;
				item << match_i;
				if (expected)
				{
					
					throw std::runtime_error( std::string( "element '") + item.str() + "' missing in boolean block");
				}
				else
				{
					throw std::runtime_error( std::string( "element '") + item.str() + "' unexpected in boolean block");
				}
			}
		}
	}
	std::cerr << "tested boolean block " << times << " times with " << minNofElements << " elements and " << nofQueries << " queries with success" << std::endl;
}


int main( int, const char**)
{
	try
	{
		initRand();
		testBooleanBlock( 100, 3000, 10000);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


