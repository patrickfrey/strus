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
#include "indexPacker.hpp"
#include "dataBlock.hpp"
#include "posinfoBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <limits>

#define RANDINT(MIN,MAX) ((rand()%(MAX+MIN))+MIN)

static std::vector<strus::Index> randPosinfo()
{
	std::vector<strus::Index> rt;
	unsigned int tt=0,nofElements=RANDINT(1,25);
	strus::Index pp = 0;
	
	for (; tt<nofElements; ++tt)
	{
		rt.push_back( pp += RANDINT(1,25));
	}
	return rt;
}


static void testDataBlockBuild( unsigned int times)
{
	unsigned int tt=0;
	std::string str( 1024, ' ');
	
	for (; tt<times; ++tt)
	{
		strus::DataBlock dblk;

		unsigned int ii=0,nofDocs=RANDINT(1,tt+2);
		for (; ii<nofDocs; ++ii)
		{
			dblk.append( str.c_str(), RANDINT(1,125));
		}
		const char* bb = (const char*)::memrchr( dblk.charptr(), 'a', dblk.size());
		if (bb == dblk.charptr())
		{
			std::cerr << "HALLY GALLY" << std::endl;
		}
	}
	std::cerr << "tested data block building " << times << " times with success" << std::endl;
}

static void testPosinfoBlockBuild( unsigned int times)
{
	unsigned int tt=0;
	
	for (; tt<times; ++tt)
	{
		strus::Index dd = 0;

		typedef std::map<strus::Index,std::vector<strus::Index> > PosinfoMap;
		PosinfoMap pmap;

		strus::PosinfoBlock block;
		unsigned int ii=0,nofDocs=RANDINT(1,100);
		for (; ii<nofDocs; ++ii)
		{
			dd += RANDINT(1,25);
			pmap[ dd] = randPosinfo();
		}
		block.setId( dd);
		PosinfoMap::const_iterator pi = pmap.begin(), pe = pmap.end();
		for (; pi != pe; ++pi)
		{
			block.append( pi->first, pi->second);
		}
	}
	std::cerr << "tested posinfo block building " << times << " times with success" << std::endl;
}


int main( int, const char**)
{
	try
	{
		testDataBlockBuild( 1);
		testPosinfoBlockBuild( 100);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


