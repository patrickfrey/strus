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
#include "strus/resultDocument.hpp"
#include "strus/queryResult.hpp"
#include "strus/base/math.hpp"
#include "strus/base/pseudoRandom.hpp"
#include "strus/base/fileio.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <algorithm>

static strus::PseudoRandom g_random;
static strus::Index g_docnum = 0;

#undef STRUS_LOWLEVEL_DEBUG

static strus::WeightedDocument randomWeightedDocument()
{
	float weight = (float)g_random.get(0,10000) / g_random.get(1,10000);
	return strus::WeightedDocument( ++g_docnum, weight);
}

static int compareWeight( double w1, double w2)
{
	if (w1 > w2 + std::numeric_limits<double>::epsilon()) return +1;
	if (w1 < w2 - std::numeric_limits<double>::epsilon()) return -1;
	return 0;
}

static void reorderSameWeightRanks( std::vector<strus::ResultDocument>& ranks)
{
	std::vector<strus::ResultDocument>::iterator ri = ranks.begin(), re = ranks.end();
	while (ri != re)
	{
		std::vector<strus::ResultDocument>::iterator rs = ri;
		for (; ri != re && 0==compareWeight(ri->weight(), rs->weight()); ++ri){}
		std::sort( rs, ri, std::greater<strus::ResultDocument>());
	}
}

static void writeRanksToFile( const char* filename, std::vector<strus::ResultDocument>& ranks)
{
	std::ostringstream out;
	std::vector<strus::ResultDocument>::iterator ri = ranks.begin(), re = ranks.end();
	for (; ri != re; ++ri)
	{
		out << ri->docno() << " " << ri->weight() << std::endl;
	}
	out << std::endl;
	int ec = strus::writeFile( filename, out.str());
	if (ec) throw std::runtime_error( ::strerror(ec));
}

int main( int , const char** )
{
	try
	{
		enum {NofWeightedDocs=50000,NofRankLists=20};
		typedef std::multiset<
				strus::WeightedDocument,
				std::greater<strus::WeightedDocument>,
				strus::LocalStructAllocator<strus::WeightedDocument> > RankSet;
		RankSet test_set;

		std::vector<std::vector<strus::ResultDocument> > test_docs( NofRankLists);
		for (std::size_t ii=0; ii<NofWeightedDocs; ++ii)
		{
			strus::WeightedDocument wd( randomWeightedDocument());
			test_set.insert( wd);
		}
		std::vector<strus::ResultDocument> EXP_ranks;
		RankSet::const_iterator ti = test_set.begin(), te = test_set.end();
		for (; ti != te; ++ti)
		{
			std::size_t ridx = g_random.get( 0, NofRankLists);
			test_docs[ ridx].push_back( *ti);
			EXP_ranks.push_back( *ti);
		}
		int expectedEvaluationPass = 0;
		std::vector<strus::QueryResult> test_input;
		for (std::size_t ai = 0; ai < NofRankLists; ++ai)
		{
			int evaluationPass = (ai % 3)/*evaluationPass*/;
			if (evaluationPass > expectedEvaluationPass) expectedEvaluationPass = evaluationPass;
			test_input.push_back( strus::QueryResult( evaluationPass, test_docs[ai].size()*2/*nofRanked*/, test_docs[ai].size()*3/*nofVisited*/, test_docs[ai]/*ranks*/));
		}
		
		strus::QueryResult RES = strus::QueryResult::merge( test_input);
		std::vector<strus::ResultDocument> RES_ranks( RES.ranks());
		reorderSameWeightRanks( RES_ranks);
		strus::QueryResult EXP( expectedEvaluationPass, EXP_ranks.size()*2/*nofRanked*/, EXP_ranks.size()*3/*nofVisited*/, EXP_ranks/*ranks*/);

		// Check results:
		if (RES.evaluationPass() != EXP.evaluationPass()) throw std::runtime_error("evaluationPass does not match");
		if (RES.nofRanked() != EXP.nofRanked()) throw std::runtime_error("nofRanked does not match");
		if (RES.nofVisited() != EXP.nofVisited()) throw std::runtime_error("nofVisited does not match");
		if (RES.ranks().size() != EXP.ranks().size()) throw std::runtime_error("size of ranklist does not match");

		std::vector<strus::ResultDocument>::const_iterator ri = RES_ranks.begin(), re = RES_ranks.end();
		std::vector<strus::ResultDocument>::const_iterator xi = EXP_ranks.begin(), xe = EXP_ranks.end();

		for (; ri != re && xi != xe; ++xi,++ri)
		{
			if (compareWeight( ri->weight(), xi->weight()) != 0)
			{
				writeRanksToFile( "RES", RES_ranks);
				writeRanksToFile( "EXP", EXP_ranks);
				throw std::runtime_error("weight of rank does not match");
			}
			if (ri->docno() != xi->docno())
			{
				writeRanksToFile( "RES", RES_ranks);
				writeRanksToFile( "EXP", EXP_ranks);
				throw std::runtime_error("docno of rank does not match");
			}
		}
		std::cerr << "OK" << std::endl;
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


