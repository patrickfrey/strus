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
#include "posinfoBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <map>

#undef STRUS_LOWLEVEL_DEBUG

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

static std::string indexVectorToString( const std::vector<strus::Index>& ar)
{
	std::ostringstream rt;
	std::vector<strus::Index>::const_iterator ai = ar.begin(), ae = ar.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) rt << ", ";
		rt << *ai;
	}
	return rt.str();
}

static void testPosinfoBlockBuild( unsigned int times, unsigned int minNofDocs)
{
	unsigned int tt=0;
	
	for (; tt<times; ++tt)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "test " << tt << ":" << std::endl;
#endif
		strus::Index dd = 0;

		typedef std::map<strus::Index,std::vector<strus::Index> > PosinfoMap;
		PosinfoMap pmap;

		std::vector<strus::PosinfoBlock> blockar;
		strus::PosinfoBlockBuilder block;
		unsigned int ii=0,nofDocs=minNofDocs+RANDINT(1,100);
		for (; ii<nofDocs; ++ii)
		{
			dd += RANDINT(1,25);
			pmap[ dd] = randPosinfo();
		}
		block.setId( dd);
		PosinfoMap::const_iterator pi = pmap.begin(), pe = pmap.end();
		for (; pi != pe; ++pi)
		{
			if (block.full() || !block.fitsInto( pi->second.size()))
			{
				blockar.push_back( block.createBlock());
				block.clear();
			}
			block.append( pi->first, pi->second);
		}
		if (!block.empty())
		{
			blockar.push_back( block.createBlock());
			block.clear();
		}
		std::vector<strus::PosinfoBlock>::const_iterator
			bi = blockar.begin(),
			be = blockar.end();
		strus::PosinfoBlock::Cursor bidx;

#ifdef STRUS_LOWLEVEL_DEBUG
		strus::Index dx = bi->firstDoc( bidx);
		while (dx)
		{
			std::vector<strus::Index> pos = bi->positions_at( bidx);
			std::cout << "\tDOC " << dx << ": " << indexVectorToString( pos) << std::endl;
			dx = bi->nextDoc( bidx);
			if (!dx)
			{
				++bi;
				if (bi != be)
				{
					dx = bi->firstDoc( bidx);
				}
			}
		}
#endif

		bi = blockar.begin();
		strus::Index dn = bi->firstDoc( bidx);

		pi = pmap.begin(), pe = pmap.end();
		while (pi != pe && bi != be)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "\tdoc " << pi->first << ":" << std::endl;
#endif
			if (dn != pi->first)
			{
				std::ostringstream msg;
				msg << dn << " != " << pi->first;
				throw std::runtime_error( std::string( "posinfo block build failed, document number mismatch: ") + msg.str());
			}
			std::vector<strus::Index> pos = bi->positions_at( bidx);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "\tpos " << indexVectorToString( pi->second) << ":" << std::endl;
#endif
			if (pos != pi->second)
			{
				throw std::runtime_error( std::string( "posinfo block build failed, mismatch in positions: {") + indexVectorToString( pos) + "} != {" + indexVectorToString( pi->second) + "}");
			}
			dn = bi->nextDoc( bidx);
			if (!dn)
			{
				++bi;
				if (bi != be)
				{
					dn = bi->firstDoc( bidx);
				}
			}
			++pi;
		}
	}
	std::cerr << "tested posinfo block building " << times << " times with " << minNofDocs << " documents with success" << std::endl;
}


int main( int, const char**)
{
	try
	{
		testDataBlockBuild( 1);
		testPosinfoBlockBuild( 100, 3000);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


