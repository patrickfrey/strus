/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "ranker.hpp"
#include "strus/index.hpp"
#include "strus/weightedDocument.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <ctime>

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}
#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)
static strus::Index g_docnum = 0;

#undef STRUS_LOWLEVEL_DEBUG

static std::string doubleToString( double val_)
{
	unsigned int val = (unsigned int)::floor( val_ * 1000);
	unsigned int val_sec = val / 1000;
	unsigned int val_ms = val % 1000;
	std::ostringstream val_str;
	val_str << val_sec << "." << std::setfill('0') << std::setw(3) << val_ms;
	return val_str.str();
}

static strus::WeightedDocument randomWeightedDocument()
{
	float weight = (float)RANDINT(0,10000) / RANDINT(1,10000);
	return strus::WeightedDocument( ++g_docnum, weight);
}

int main( int , const char** )
{
	try
	{
		initRand();

		enum {MaxNofRanks=10,NofWeightedDocs=1000000};
		strus::Ranker ranker( MaxNofRanks);
		typedef std::multiset<
				strus::WeightedDocument,
				std::less<strus::WeightedDocument>,
				strus::LocalStructAllocator<strus::WeightedDocument> > RankSet;
		RankSet test_set;

		std::vector<strus::WeightedDocument> test_docs;

		for (std::size_t ii=0; ii<NofWeightedDocs; ++ii)
		{
			strus::WeightedDocument wd( randomWeightedDocument());
			test_docs.push_back( wd);
		}

		// Build and measure reference ranklist based on multiset:
		std::clock_t start;
		double duration;
		start = std::clock();

		for (std::size_t ii=0; ii<NofWeightedDocs; ++ii)
		{
			test_set.insert( test_docs[ii]);
			if (MaxNofRanks < ii)
			{
				test_set.erase( test_set.begin());
			}
		}
		std::vector<strus::WeightedDocument> testlist;
		std::set<strus::WeightedDocument>::reverse_iterator si = test_set.rbegin(), se = test_set.rend();
		for (int sidx=0; si != se && sidx < MaxNofRanks; ++si,++sidx)
		{
			testlist.push_back( *si);
		}

		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "reference ranking of " << (int)NofWeightedDocs << " documents in " << doubleToString(duration) << " seconds" << std::endl;

		// Build and measure ranklist based on ranker:
		start = std::clock();
		for (std::size_t ii=0; ii<NofWeightedDocs; ++ii)
		{
			ranker.insert( test_docs[ii]);
		}
		std::vector<strus::WeightedDocument> ranklist = ranker.result( 0);
		duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
		std::cerr << "ranker ranking of " << (int)NofWeightedDocs << " documents in " << doubleToString(duration) << " seconds" << std::endl;

		// Check results:
		std::vector<strus::WeightedDocument>::const_iterator ri = ranklist.begin(), re = ranklist.end();
		std::vector<strus::WeightedDocument>::const_iterator ti = testlist.begin(), te = testlist.end();

		int ridx=0;

#ifdef STRUS_LOWLEVEL_DEBUG
		int tidx=0;

		for (ridx=0,ri=ranklist.begin(); ri != re; ++ri,++ridx)
		{
			std::cerr << "result [" << ridx << "] "
					<< " docno " << ri->docno()
					<< " weight "<< ri->weight()
					<< std::endl;
		}
		for (tidx=0,ti=testlist.begin(); ti != te; ++ti,++tidx)
		{
			std::cerr << "expect [" << tidx << "] "
					<< " docno " << ti->docno()
					<< " weight "<< ti->weight()
					<< std::endl;
		}
#endif
		int rt = 0;
		for (ridx=0,ri=ranklist.begin(),ti=testlist.begin(); ri != re && ti != te; ++ri,++ti,++ridx)
		{
			if (ti->weight() != ri->weight() || ti->docno() != ri->docno())
			{
				std::cerr << "rank does not match [" << ridx << "] "
						<< " docno " << ti->docno() << "/" << ri->docno()
						<< " weight " << ti->weight() << "/" << ri->weight()
						<< std::endl;
				rt = 1;
			}
		}
		return rt;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


