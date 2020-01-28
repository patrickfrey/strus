/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "proximityWeightingContext.hpp"
#include "strus/structIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include <cstring>

using namespace strus;

#define STRUS_LOWLEVEL_DEBUG

void ProximityWeightingContext::init(
	PostingIteratorInterface** postings, int nofPostings, int minClusterSize,
	PostingIteratorInterface* eos_postings, strus::Index docno)
{
	if (nofPostings >= MaxNofArguments)
	{
		throw strus::runtime_error(_TXT("query to complex, number of features (%d) out of range (allowed %d)"), nofPostings, (int)MaxNofArguments);
	}
	PostingIteratorInterface* par[ MaxNofArguments];
	strus::Index posnoar[ MaxNofArguments];		//... array parallel to par
	unsigned char prioar[ MaxNofArguments];		//... array with indices into par and posnoar

	par[ 0] = eos_postings;
	int pidx=1;
	int pi=0;
	for (;pi<nofPostings; ++pi)
	{
		if (postings[ pi]->skipDoc(docno) == docno)
		{
			par[ pidx] = postings[ pi];
			posnoar[ pidx] = par[ pi]->skipPos( 0);
			if (posnoar[ pidx])
			{
				pidx++;
			}
		}
	}
	int parsize = pidx;

	for (pi=0; pi<parsize; ++pi)
	{
		int ki = 0;
		for (; ki<pi && posnoar[ki] < posnoar[ pi]; ++ki){}
		if (ki < pi)
		{
			std::memove( prioar+ki+1, prioar+ki, pi-ki);
		}
		prioar[ ki] = pi;
	}
	int quesize = parsize;
	int min_quesize = minClusterSize > 1 ? minClusterSize : 2;
	while (quesize >= min_quesize)
	{
		int featidx = prioar[ 0];
		strus::Index documentPos = posnoar[ featidx];
		m_nodear.push_back( Node( featidx, documentPos));
		strus::Index nextPos = (featidx == 0)//... is punctuation
				? posnoar[ prioar[ 1]];
				: posnoar[ prioar[ min_quesize-1]];
		if (nextPos == documentPos)
		{
			++nextPos;
		}
		posnoar[ featidx] = par[ featidx]>skipPos( nextPos);
		if (posnoar[ featidx] == 0)
		{
			std::memove( prioar, prioar+1, quesize-1);
			quesize--;
		}
		else
		{
			int ki = 0;
			for (; ki<quesize && posnoar[ki] < nextPos; ++ki)
			{
				prioar[ ki] = prioar[ ki+1];
			}
			if (ki)
			{
				prioar[ ki-1] = featidx;
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		{
			std::set<int> check_queue_set;
			for (int ki=0; ki<quesize; ++ki)
			{
				if (ki && posnoar[ prioar[ ki-1] ] > posnoar[ prioar[ ki] ])
				{
					throw std::runtime_error(_TXT("logic error: queue corrupted in proximity weighting (element order)"));
				}
				check_queue_set.insert( prioar[ ki]);
			}
			if (check_queue_set.size() != quesize)
			{
				throw std::runtime_error(_TXT("logic error: queue corrupted in proximity weighting (duplicates)"));
			}
		}
#endif
	}
	if (minClusterSize == 1)
	{
		int featidx = prioar[ 0];
		strus::Index documentPos = posnoar[ featidx];
		for (; documentPos; documentPos = par[ featidx]>skipPos( documentPos+1))
		{
			m_nodear.push_back( Node( featidx, documentPos));
		}
	}
	else if (quesize)
	{
		int featidx = prioar[ 0];
		strus::Index documentPos = posnoar[ featidx];
		m_nodear.push_back( Node( featidx, documentPos));
	}
}

void ProximityWeightingContext::cluster( const Config& config)
{
	// Mark sentence relations (imm,close,sentence):
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->featidx == 0/*eos*/) continue;

		std::vector<Node>::iterator fi = ni+1, fe = m_nodear.end();
		for (; fi != fe && fi->featidx != 0/*eos*/ && ni->pos + config.distance_near < fi->pos; ++fi)
		{
			if (ni->pos + config.distance_close > fi->pos)
			{
				if (ni->pos + config.distance_imm > fi->pos)
				{
					if (fi->touched.set( ni->featidx))
					{
						++fi->immFollowMatches;
					}
					if (ni->touched.set( fi->featidx))
					{
						++ni->closeMatches; //... immediate matches only for follows !
					}
				}
				else
				{
					if (fi->touched.set( ni->featidx))
					{
						++fi->closeMatches;
					}
					if (ni->touched.set( fi->featidx))
					{
						++ni->closeMatches;
					}
				}
			}
			else
			{
				if (fi->touched.set( ni->featidx))
				{
					++fi->sentenceMatches;
				}
				if (ni->touched.set( fi->featidx))
				{
					++ni->sentenceMatches;
				}
			}
		}
	}
	// Mark near relations not in same sentence:
	ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->featidx == 0/*eos*/) continue;

		std::vector<Node>::iterator fi = ni+1, fe = m_nodear.end();
		for (; fi != fe && ni->pos + config.distance_near < fi->pos; ++fi)
		{
			if (ni->featidx != 0)
			{
				if (fi->touched.set( ni->featidx))
				{
					++fi->nearMatches;
				}
				if (ni->touched.set( fi->featidx))
				{
					++ni->nearMatches;
				}
			}
		}
	}
}



