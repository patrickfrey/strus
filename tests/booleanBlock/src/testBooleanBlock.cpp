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
#include "strus/index.hpp"
#include "indexPacker.hpp"
#include "dataBlock.hpp"
#include "booleanBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <set>

#undef STRUS_LOWLEVEL_DEBUG

#define RANDINT(MIN,MAX) ((rand()%(MAX+MIN))+MIN)


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
		testBooleanBlock( 100, 3000, 10000);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


