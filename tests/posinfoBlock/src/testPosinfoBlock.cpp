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
#include "posinfoBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <map>

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

static void testPosinfoBlock( unsigned int times, unsigned int minNofDocs, unsigned int nofQueries)
{
	unsigned int tt=0;
	
	for (; tt<times; ++tt)
	{
		strus::Index maxDocNo = 0;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "test " << tt << ":" << std::endl;
#endif
		typedef std::map<strus::Index,std::vector<strus::Index> > PosinfoMap;
		PosinfoMap pmap;
		std::vector<strus::Index> docnoar;

		std::vector<strus::PosinfoBlock> blockar;
		strus::PosinfoBlockBuilder block;
		unsigned int ii=0,nofDocs=minNofDocs+RANDINT(1,100);
		for (; ii<nofDocs; ++ii)
		{
			maxDocNo += RANDINT(1,25);
			pmap[ maxDocNo] = randPosinfo();
			docnoar.push_back( maxDocNo);
		}
		PosinfoMap::const_iterator pi = pmap.begin(), pe = pmap.end();
		for (; pi != pe; ++pi)
		{
			if (block.full() || !block.fitsInto( pi->second.size()))
			{
				blockar.push_back( block.createBlock());
				block.clear();
			}
			std::vector<strus::PosinfoBlock::PositionType> posar;
			std::vector<strus::Index>::const_iterator
				ci = pi->second.begin(), ce = pi->second.end();
			posar.push_back( pi->second.size());
			for (; ci != ce; ++ci)
			{
				posar.push_back( *ci);
			}
			block.append( pi->first, posar.data());
		}
		if (!block.empty())
		{
			blockar.push_back( block.createBlock());
			block.clear();
		}
		std::vector<strus::PosinfoBlock>::iterator
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

		unsigned int qi = 0, qe = nofQueries;
		for (; qi != qe; ++qi)
		{
			std::size_t didx = RANDINT(0,docnoar.size());
			strus::Index dn = docnoar[ didx];
			strus::Index find_dn = dn;
			if (didx > 0 && docnoar[ didx-1] +1 < dn && RANDINT(0,3) == 1)
			{
				find_dn = dn - 1;
			}
			PosinfoMap::const_iterator di = pmap.find( dn);
			if (di == pmap.end()) throw std::logic_error("bad things happened");
			std::size_t pidx = 0;
			strus::Index pos = di->second.size()?di->second[ pidx=RANDINT(0,di->second.size())]:0;
			strus::Index find_pos = pos;
			if (pidx > 0 && di->second[ pidx-1] + 1 < pos && RANDINT(0,3) == 1)
			{
				find_pos = pos - 1;
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "try query skipDoc(" << find_dn << ") => " << dn << ", skipPos( " << find_pos << ") => " << pos << std::endl;
#endif
			for (bi = blockar.begin(); bi != be; ++bi)
			{
				if (bi->id() >= dn)
				{
					strus::PosinfoBlock::Cursor cursor;
					if (dn != bi->skipDoc( find_dn, cursor))
					{
						throw std::runtime_error( "posinfo block skip doc query failed");
					}
					strus::PosinfoBlock::PositionScanner
						scan = bi->positionScanner_at( cursor);
					
					if (scan.initialized())
					{
						if (pos != scan.skip( find_pos))
						{
							throw std::runtime_error( "posinfo block skip pos query failed");
						}
					}
					else
					{
						if (pos)
						{
							throw std::runtime_error( "positions expected but not available");
						}
					}
					break;
				}
			}
			if (bi == be)
			{
				throw std::runtime_error("block for posinfo test query not found");
			}
		}
	}
	std::cerr << "tested posinfo block " << times << " times with " << minNofDocs << " documents with success" << std::endl;
}


int main( int, const char**)
{
	try
	{
		initRand();
		testDataBlockBuild( 1);
		testPosinfoBlock( 100, 3000, 1000);
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


