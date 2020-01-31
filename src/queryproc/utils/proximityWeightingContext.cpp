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
#include "strus/base/static_assert.hpp"
#include <cstring>
#include <set>

using namespace strus;

#ifdef NDEBUG
#undef STRUS_LOWLEVEL_DEBUG
#else
#define STRUS_LOWLEVEL_DEBUG
#endif

void ProximityWeightingContext::init(
	PostingIteratorInterface** postings, int nofPostings, const Config& config,
	PostingIteratorInterface* eos_postings, strus::Index docno, const strus::IndexRange& field)
{
	if (nofPostings == 0)
	{
		throw strus::runtime_error(_TXT("query is empty"));
	}
	m_config = config;
	m_nofPostings = 0;
	m_nofWeightedNeighbours = nofPostings-1;
	m_docno = docno;
	m_field = field;
	int eos_featidx = -1;

	if (eos_postings && eos_postings->skipDoc(docno) == docno)
	{
		eos_featidx = 0;
		m_hasPunctuation = true;
		m_postings[ m_nofPostings++] = eos_postings; 
		if (nofPostings >= MaxNofArguments)
		{
			throw strus::runtime_error(_TXT("query to complex, number of features (%d) out of range (allowed %d)"), nofPostings+1, (int)MaxNofArguments);
		}
	}
	else
	{
		m_hasPunctuation = false;
		if (nofPostings > MaxNofArguments)
		{
			throw strus::runtime_error(_TXT("query to complex, number of features (%d) out of range (allowed %d)"), nofPostings, (int)MaxNofArguments);
		}
	}
	m_nodear.clear();
	m_stmOperations.clear();
	m_fieldStatistics.clear();
	m_titleFields.clear();
	m_stmStack.clear();
	m_nodeScanner.init( 0, 0);

	strus::Index endpos = m_field.defined() ? m_field.end() : std::numeric_limits<Position>::max();

	strus::Index posnoar[ MaxNofArguments];		//... array parallel to m_postings
	unsigned char prioar[ MaxNofArguments];		//... array with indices into m_postings and posnoar

	int pi=0;
	for (;pi<nofPostings; ++pi)
	{
		if (postings[ pi]->skipDoc(docno) == docno)
		{
			m_postings[ m_nofPostings] = postings[ pi];
			posnoar[ m_nofPostings] = m_postings[ pi]->skipPos( field.start());
			if (0 == posnoar[ m_nofPostings] || posnoar[ m_nofPostings] > endpos)
			{
				m_postings[ m_nofPostings] = 0;
			}
		}
		else
		{
			m_postings[ m_nofPostings] = 0;
		}
		++m_nofPostings;
	}
	int quesize = 0;
	for (pi=0; pi<m_nofPostings; ++pi)
	{
		if (posnoar[ pi])
		{
			int ki = 0;
			for (; ki<quesize && posnoar[ki] < posnoar[ pi]; ++ki){}
			if (ki < pi)
			{
				std::memmove( prioar+ki+1, prioar+ki, pi-ki);
			}
			prioar[ ki] = pi;
			++quesize;
		}
	}
	int minClusterSize = (int)(config.minClusterSize * nofPostings + 0.5);
	int min_quesize = minClusterSize > 1 ? minClusterSize : 2;
	while (quesize >= min_quesize)
	{
		int featidx = prioar[ 0];
		Position documentPos = (Position)(unsigned int)(int)posnoar[ featidx];
		m_nodear.push_back( Node( featidx, documentPos));
		strus::Index nextPos = featidx == eos_featidx
				? posnoar[ prioar[ 1]]
				: posnoar[ prioar[ min_quesize-1]];
		if (nextPos == documentPos)
		{
			++nextPos;
		}
		posnoar[ featidx] = m_postings[ featidx]->skipPos( nextPos);
		if (posnoar[ featidx] == 0 || posnoar[ featidx] >= endpos)
		{
			std::memmove( prioar, prioar+1, quesize-1);
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
			if ((int)check_queue_set.size() != quesize)
			{
				throw std::runtime_error(_TXT("logic error: queue corrupted in proximity weighting (duplicates)"));
			}
		}
#endif
	}
	if (quesize)
	{
		int featidx = prioar[ 0];
		if (featidx != eos_featidx)
		{
			if (minClusterSize == 1)
			{
				// ... there is only one feature left, so we add all instances of it
				strus::Index documentPos = posnoar[ featidx];
				for (; documentPos && documentPos < endpos; documentPos = m_postings[ featidx]->skipPos( documentPos+1))
				{
					m_nodear.push_back( Node( featidx, documentPos));
				}
			}
			else
			{
				// ... add the next value in the queue as last element
				strus::Index documentPos = posnoar[ featidx];
				m_nodear.push_back( Node( featidx, documentPos));
			}
		}
	}
	initNeighbourMatches();
	m_nodeScanner.init( m_nodear.data(), m_nodear.size());
}


void ProximityWeightingContext::initNeighbourMatches()
{
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	// Mark sentence relations (imm,close,sentence):
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->featidx == eos_featidx) continue;

		std::vector<Node>::iterator fi = ni+1, fe = m_nodear.end();
		for (; fi != fe && fi->featidx != eos_featidx && ni->pos + m_config.distance_near < fi->pos; ++fi)
		{
			if (fi->featidx == ni->featidx) continue;

			if (ni->pos + m_config.distance_close > fi->pos)
			{
				if (ni->pos + m_config.distance_imm > fi->pos)
				{
					if (fi->touched.set( ni->featidx, true))
					{
						++fi->immediateMatches;
					}
					if (ni->touched.set( fi->featidx, true))
					{
						++ni->immediateMatches;
					}
				}
				else
				{
					if (fi->touched.set( ni->featidx, true))
					{
						++fi->closeMatches;
					}
					if (ni->touched.set( fi->featidx, true))
					{
						++ni->closeMatches;
					}
				}
			}
			else
			{
				if (fi->touched.set( ni->featidx, true))
				{
					++fi->sentenceMatches;
				}
				if (ni->touched.set( fi->featidx, true))
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
		if (ni->featidx == eos_featidx) continue;

		std::vector<Node>::iterator fi = ni+1, fe = m_nodear.end();
		for (; fi != fe && ni->pos + m_config.distance_near < fi->pos; ++fi)
		{
			if (fi->featidx == ni->featidx || ni->featidx == eos_featidx) continue;

			if (fi->touched.set( ni->featidx, true))
			{
				++fi->nearMatches;
			}
			if (ni->touched.set( fi->featidx, true))
			{
				++ni->nearMatches;
			}
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->touchCount() > (m_nofPostings - (m_hasPunctuation ? 1 : 0)))
		{
			throw std::runtime_error(_TXT("logic error: proximity weighting touch count exceeds number of features"));
		}
	}
#endif
}

double ProximityWeightingContext::ff_weight( const Node& nd) const
{
	struct TouchCountWeightMap
	{
	public:
		TouchCountWeightMap()
		{
			std::memset( &m_ar, 0, sizeof(m_ar));
			for (int ri=1; ri<ProximityWeightingContext::MaxNofArguments; ++ri)
			{
				m_ar[ ri][ ri] = 1.0;
				for (int ci=ri-1; ci>0; --ci)
				{
					m_ar[ ri][ ci] = (float)(ci * ci)/(float)(ri * ri);
				}
				m_ar[ ri][ 0] = 0.0;
			}
		}
		float const* get( int nofWeightedNeighbours) const
		{
			return m_ar[ nofWeightedNeighbours];
		}
	private:
		float m_ar[ ProximityWeightingContext::MaxNofArguments][ ProximityWeightingContext::MaxNofArguments];
	};
	static TouchCountWeightMap g_touchCountWeightMap;

	float const* wmap = g_touchCountWeightMap.get( m_nofWeightedNeighbours);
	double ww =  0.0;
	if (nd.immediateMatches) ww += 1.0;
	int wi = nd.immediateMatches + nd.closeMatches;
	ww += wmap[ wi];
	wi += nd.sentenceMatches;
	ww += wmap[ wi];
	wi += nd.nearMatches + nd.titleScopeMatches;
	ww += wmap[ wi];

	return (1.0 - m_config.minFfWeight) * (ww/4) + m_config.minFfWeight;
}

void ProximityWeightingContext::touchTitleNode( std::vector<Node>::iterator ni, const strus::IndexRange& headerField, const strus::IndexRange& contentField)
{
	std::vector<Node>::iterator start = ni;
	for (; start >= m_nodear.begin() && start->pos >= (Position)contentField.start(); --start){}
	++start;
	std::vector<Node>::iterator end = ni;
	for (; end != m_nodear.end() && end->pos < (Position)contentField.end(); ++end){}
	strus::bitset<MaxNofArguments> title_touched;
	int ti = m_hasPunctuation ? 1 : 0, te = m_nofPostings;
	for (; ti != te; ++ti)
	{
		if (m_postings[ ti])
		{
			strus::Index pos = m_postings[ ti]->skipPos( headerField.start());
			if (pos && pos < headerField.end())
			{
				title_touched.set( ti, true);
			}
		}
	}
	for (ni=start; ni<end; ++ni)
	{
		int tc = ni->touched.join_count( title_touched);
		ni->titleScopeMatches += tc;
	}
}

void ProximityWeightingContext::initStructures( StructIteratorInterface* structIterator)
{
	m_stmOperations.clear();
	m_fieldStatistics.clear();
	m_stmStack.clear();
	int nofCandidates[MaxNofArguments];
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	structIterator->skipDoc( m_docno);
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		int tc = ni->touchCount();
		++nofCandidates[ tc];
	}
	{
		int sum = 0;
		int tc = m_nofPostings;
		for (;tc > 0;--tc)
		{
			sum += nofCandidates[ tc];
			if (sum >= m_config.nofHotspots) break;
		}
		int li=0, le=structIterator->levels();
		for (; li != le; ++li)
		{
			ni = m_nodear.begin();
			while (ni != ne)
			{
				if (ni->touchCount() >= tc && ni->featidx != eos_featidx)
				{
					strus::IndexRange field = structIterator->skipPos( li, ni->pos);
					if (field.contain( ni->pos))
					{
						bool fieldUsed = false;
						StructureLinkArray lar = structIterator->links( li);
						int ki=0, ke=lar.nofLinks();
						for (; ki!=ke;++ki)
						{
							const StructureLink& lnk = lar[ ki];
							if (!lnk.header())
							{
								if (!fieldUsed)
								{
									if (!m_config.structno || m_config.structno == lnk.structno())
									{
										fieldUsed = true;
										m_stmOperations.push_back( StmOperation( field.start(), m_fieldStatistics.size()));
										m_stmOperations.push_back( StmOperation( field.start(), field.end(), m_fieldStatistics.size()));
										m_fieldStatistics.push_back( FieldStatistics( field));
									}
								}
								strus::IndexRange headerField = structIterator->headerField( lnk.index());
								touchTitleNode( ni, headerField, field);

								if (m_field.defined() && field.cover( m_field))
								{
									m_titleFields.push_back( headerField);
								}
							}
						}
					}
					int nextidx = m_nodeScanner.upperbound( field.end(), ni-m_nodear.begin(), m_nodear.size());
					if (nextidx > 0)
					{
						ni += nextidx;
					}
					else
					{
						ni = ne;
					}
				}
				else
				{
					++ni;
				}
			}
		}
		std::sort( m_stmOperations.begin(), m_stmOperations.end());
	}
}

void ProximityWeightingContext::collectFieldStatistics()
{
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;
	unsigned char ofs_featidx = m_hasPunctuation ? 1 : 0;

	m_stmStack.clear();

	if (m_stmOperations.empty())
	{
		m_fieldStatistics.clear();
		m_fieldStatistics.resize(1);
		FieldStatistics& res = m_fieldStatistics[0];

		std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
		for (; ni != ne; ++ni)
		{
			if (ni->featidx)
			{
				res.ff[ ni->featidx-ofs_featidx] += ff_weight( *ni);
			}
		}
	}
	else
	{
		std::vector<StmOperation>::const_iterator
			oi = m_stmOperations.begin(), oe = m_stmOperations.end();
		std::vector<Node>::const_iterator
			ni = m_nodear.begin(), ne = m_nodear.end();

		while (oi != oe && ni != ne)
		{
			for (; oi != oe && oi->endpos <= ni->pos; ++oi)
			{
				//... structure operations on the stack of currently open fields
				if (oi->open())
				{
					m_stmStack.push_back( oi->fieldidx);
				}
				else if (!m_stmStack.empty() && oi->fieldidx == m_stmStack.back())
				{
					m_stmStack.pop_back();
				}
				else
				{
					throw std::runtime_error(_TXT("logic error: unbalanced open/close on field stack"));
				}
			}
			if (m_stmStack.empty() && oi != oe)
			{
				//... No fields on the stack, skip forward to next start of a field
				int nidx = m_nodeScanner.upperbound( oi->endpos, ni-m_nodear.begin(), m_nodear.size());
				if (nidx > 0)
				{
					ni = ni+nidx;
					continue;
				}
			}
			for (; ni != ne && oi != oe && ni->pos < oi->endpos; ++ni)
			{
				if (ni->featidx != eos_featidx)
				{
					//... do accumulate weights for the open fields
					double ww = ff_weight( *ni);
	
					std::vector<int>::const_iterator
						ki = m_stmStack.begin(), ke = m_stmStack.end();
					for (; ki != ke; ++ki)
					{
						m_fieldStatistics[ *ki].ff[ ni->featidx - ofs_featidx] += ww;
					}
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		while (oi != oe)
		{
			//... structure operations on the stack of currently open fields
			if (oi->open())
			{
				m_stmStack.push_back( oi->fieldidx);
			}
			else if (!m_stmStack.empty() && oi->fieldidx == m_stmStack.back())
			{
				m_stmStack.pop_back();
			}
			else
			{
				throw std::runtime_error(_TXT("logic error: unbalanced open/close on field stack"));
			}
		}
#endif
	}
}

strus::IndexRange ProximityWeightingContext::getBestPassage( const FeatureWeights& featureWeights) const
{
	STRUS_STATIC_ASSERT( MaxSummaryFieldSize != 0
			&& (MaxSummaryFieldSize & (MaxSummaryFieldSize-1)) == 0);
	enum {MaxSummaryFieldMask = MaxSummaryFieldSize-1};

	strus::IndexRange rt;

	struct WeightedPos
	{
		double weight;
		strus::Index pos;
		strus::Index sentidx;
	};
	WeightedPos weightedPos[ MaxSummaryFieldSize];
	std::memset( &weightedPos, 0, sizeof(weightedPos));

	double weightsum = 0.0;
	double weightsum_max = 0.0;
	int sentidx = 0;
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	int nofPositions = m_config.nofSentencesSummary * m_config.maxNofSentenceWords;
	if (nofPositions >= MaxSummaryFieldSize) nofPositions = MaxSummaryFieldSize-1;

	int cur_idx = 0;
	int start_idx = 0;
	std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
	while (ni != ne)
	{
		if (ni->featidx == eos_featidx)
		{
			++sentidx;
			++ni;
			continue;
		}
		WeightedPos& cur = weightedPos[ cur_idx];
		cur.pos = ni->pos;
		cur.sentidx = sentidx;
		cur.weight = featureWeights[ ni->featidx] * ff_weight( *ni);

		for (++ni; ni != ne && ni->pos == cur.pos; ++ni)
		{
			cur.weight += featureWeights[ ni->featidx] * ff_weight( *ni);
		}
		for (; start_idx != cur_idx
			&& weightedPos[ start_idx].pos + nofPositions < ni->pos;
			start_idx = (start_idx + 1) & MaxSummaryFieldMask)
		{
			weightsum -= weightedPos[ start_idx].weight;
		}
		weightsum += cur.weight;
		cur_idx = (cur_idx + 1) & MaxSummaryFieldMask;

		if (weightsum > weightsum_max)
		{
			weightsum_max = weightsum;
			if (m_hasPunctuation)
			{
				int sidx = start_idx;
				while (weightedPos[ sidx].sentidx + m_config.nofSentencesSummary
						< weightedPos[ cur_idx].sentidx)
				{
					++sidx;
				}
				rt.init( weightedPos[ sidx].pos, weightedPos[ cur_idx].pos+1);
			}
			else
			{
				rt.init( weightedPos[ start_idx].pos, weightedPos[ cur_idx].pos+1);
			}
		}
	}
	return rt;
}


