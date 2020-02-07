/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "proximityWeightingContext.hpp"
#include "forwardIndexCollector.hpp"
#include "strus/structIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/static_assert.hpp"
#include <limits>
#include <set>

using namespace strus;

#ifdef NDEBUG
#undef STRUS_LOWLEVEL_DEBUG
#else
#define STRUS_LOWLEVEL_DEBUG
#endif

ProximityWeightingContext::ProximityWeightingContext( const ProximityWeightingContext& o)
	:m_config(o.m_config)
	,m_nofPostings(o.m_nofPostings),m_nofWeightedNeighbours(o.m_nofWeightedNeighbours)
	,m_minClusterSize(o.m_minClusterSize)
	,m_hasPunctuation(o.m_hasPunctuation),m_docno(o.m_docno),m_field(o.m_field)
	,m_nodear(o.m_nodear)
	,m_stmOperations(o.m_stmOperations)
	,m_fieldStatistics(o.m_fieldStatistics)
	,m_stmStack(o.m_stmStack)
{
	std::memcpy( m_postings, o.m_postings, m_nofPostings * sizeof(m_postings[0]));
	std::memcpy( m_featureids, o.m_featureids, m_nofPostings * sizeof(m_featureids[0]));
	std::memcpy( m_length_postings, o.m_length_postings, m_nofPostings * sizeof(m_length_postings[0]));
	m_nodeScanner.init( m_nodear.data(), m_nodear.size());
}

ProximityWeightingContext::ProximityWeightingContext( const Config& config_)
	:m_config(config_)
	,m_nofPostings(0),m_nofWeightedNeighbours(0),m_minClusterSize(0)
	,m_hasPunctuation(false),m_docno(0),m_field()
	,m_nodear()
	,m_stmOperations()
	,m_fieldStatistics()
	,m_stmStack()
	,m_nodeScanner()
{
	std::memset( m_postings, 0, MaxNofArguments * sizeof(m_postings[0]));
	std::memset( m_featureids, 0, MaxNofArguments * sizeof(m_featureids[0]));
	std::memset( m_length_postings, 0, MaxNofArguments * sizeof(m_length_postings[0]));
}

static std::size_t indexStringArray( const char** ar, std::size_t arsize, std::size_t searchstart, const char* needle)
{
	std::size_t aridx = searchstart;
	for (; aridx < arsize && 0!=std::strcmp( ar[ aridx], needle); ++aridx){}
	return aridx;
}

void ProximityWeightingContext::init(
	PostingIteratorInterface** postings, int nofPostings, PostingIteratorInterface* eos_postings,
	strus::Index docno, const strus::IndexRange& field)
{
	// [1] Basic data initialization for the current document:
	if (nofPostings == 0)
	{
		throw strus::runtime_error(_TXT("query is empty"));
	}
	if (nofPostings >= MaxNofArguments)
	{
		throw strus::runtime_error(_TXT("query to complex, number of features (%d) out of range (allowed %d)"), nofPostings, (int)MaxNofArguments);
	}
	m_nofPostings = 0;
	m_nofWeightedNeighbours = nofPostings-1;
	m_minClusterSize = (int)((m_config.minClusterSize * nofPostings) + 0.5);
	if (m_minClusterSize == 0) m_minClusterSize = 1;
	m_docno = docno;
	m_field = field;
	int eos_featidx = -1;
	int firstPostingIdx = 0;
	strus::Index featureIdToStartMap[ MaxNofArguments];	// map feature id (value in m_featureids) to the next value to take as minimum value for the start of the follow position scan
	const char* featureIdToNameMap[ MaxNofArguments];	// maps the feature id to string, used to build

	if (true == (m_hasPunctuation = (eos_postings != NULL && eos_postings->skipDoc(docno) == docno)))
	{
		eos_featidx = 0;
		m_length_postings[ m_nofPostings] = eos_postings->length();
		m_postings[ m_nofPostings] = eos_postings;
		m_featureids[ m_nofPostings] = 0;
		featureIdToStartMap[ m_nofPostings] = field.start();
		featureIdToNameMap[ m_nofPostings] = 0;
		++m_nofPostings;
		firstPostingIdx = ++m_nofPostings;
	}
	m_nodear.clear();
	m_stmOperations.clear();
	m_fieldStatistics.clear();
	m_stmStack.clear();
	m_nodeScanner.init( 0, 0);

	// [2] Inialize features for the sliding window scan:
	strus::Index endpos = m_field.defined() ? m_field.end() : (strus::Index)std::numeric_limits<Position>::max();
	strus::Index posnoar[ MaxNofArguments];		//... array parallel to m_postings
	unsigned char prioar[ MaxNofArguments];		//... array with indices into m_postings and posnoar

	int pi=0;
	for (;pi<nofPostings; ++pi)
	{
		m_postings[ m_nofPostings] = postings[ pi];
		m_length_postings[ m_nofPostings] = m_postings[ m_nofPostings]->length();
		m_featureids[ m_nofPostings] = m_nofPostings;
		int firstAppearanceIndex = indexStringArray( featureIdToNameMap, m_nofPostings, firstPostingIdx, featureIdToNameMap[ m_nofPostings]);
		int featid = m_featureids[ m_nofPostings] = m_featureids[ firstAppearanceIndex];
		featureIdToNameMap[ m_nofPostings] = m_postings[ m_nofPostings]->featureid();
		featureIdToStartMap[ m_nofPostings] = field.start();

		if (m_postings[ m_nofPostings]->skipDoc(docno) == docno)
		{
			posnoar[ m_nofPostings]
				= m_postings[ m_nofPostings]->skipPos( featureIdToStartMap[ featid]);

			if (0 == posnoar[ m_nofPostings] || posnoar[ m_nofPostings] > endpos)
			{
				m_postings[ m_nofPostings] = 0;
				featureIdToStartMap[ featid] = endpos;
			}
			else
			{
				featureIdToStartMap[ featid] = posnoar[ m_nofPostings]+1;
			}
		}
		else
		{
			m_postings[ m_nofPostings] = 0;
			posnoar[ m_nofPostings] = endpos;
			featureIdToStartMap[ featid] = endpos;
		}
		++m_nofPostings;
	}
	// [3] Initialize the queue for the sliding window scan:
	int quesize = 0;
	for (pi=0; pi<m_nofPostings; ++pi)
	{
		if (m_postings[ pi])
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
	// [4] Generate node array with sliding window scan:
	int min_quesize = m_minClusterSize + (m_hasPunctuation ? 1 : 0);
	while (quesize >= min_quesize)
	{
		// Take first feature from queue and reinsert it:
		int fidx = prioar[ 0];
		Position documentPos = (Position)(unsigned int)(int)posnoar[ fidx];
		m_nodear.push_back( Node( fidx, documentPos));

		// Get the next start position of the top element in the queue
		strus::Index nextPos;
		if (fidx == eos_featidx)
		{
			nextPos = posnoar[ prioar[ 1]];
		}
		else
		{
			nextPos = featureIdToStartMap[ m_featureids[ fidx]];
			if (quesize > 1)
			{
				strus::Index pp = posnoar[ prioar[ min_quesize-1]];
				if (nextPos + m_config.distance_near < pp)
				{
					nextPos = pp - m_config.distance_near;
				}
			}
		}
		if (nextPos <= documentPos)
		{
			throw std::runtime_error(_TXT("logic error: proximity weighting queue corrupted"));
		}
		posnoar[ fidx] = m_postings[ fidx]->skipPos( nextPos);

		if (posnoar[ fidx] == 0 || posnoar[ fidx] >= endpos)
		{
			// Set start position at end of all:
			featureIdToStartMap[ m_featureids[ fidx]] = endpos;

			// Delete this element from queue:
			std::memmove( prioar, prioar+1, quesize-1);
			quesize--;
		}
		else
		{
			// Set new start position of this feature:
			featureIdToStartMap[ m_featureids[ fidx]] = posnoar[ fidx]+1;

			// Reinsert in queue at new position:
			int ki = 1;
			for (; ki<quesize && posnoar[ prioar[ki] ] < posnoar[ fidx]; ++ki)
			{
				prioar[ ki-1] = prioar[ ki];
			}
			prioar[ ki-1] = fidx;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		{
			// Verify validity in debug mode:
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
		for (; fi != fe && fi->featidx != eos_featidx
			&& ni->pos + m_length_postings[ ni->featidx] + m_config.maxNofSummarySentenceWords > fi->pos; ++fi)
		{
			if (fi->featidx == ni->featidx) continue;

			if (ni->pos + m_length_postings[ ni->featidx] + m_config.distance_close > fi->pos)
			{
				if (ni->pos + m_length_postings[ ni->featidx] + m_config.distance_imm > fi->pos)
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
		for (; fi != fe && ni->pos + m_length_postings[ ni->featidx] + m_config.distance_near > fi->pos; ++fi)
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
	// Some validity verification when debugging:
	ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->touchCount() > (m_nofPostings - (m_hasPunctuation ? 1 : 0)))
		{
			throw std::runtime_error(_TXT("logic error: proximity weighting touch count exceeds number of features"));
		}
	}
#endif
	// Eliminate nodes with a touch count lower than the minimum window size:
	ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne && m_minClusterSize <= ni->touchCount(); ++ni){}
	if (ni != ne)
	{
		std::vector<Node>::iterator prev_ni = ni;
		for (++ni; ni != ne; ++ni)
		{
			if (m_minClusterSize <= ni->touchCount())
			{
				*prev_ni++ = *ni;
			}
		}
		m_nodear.resize( prev_ni - m_nodear.begin());
	}
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
				for (int ci=ri; ci>=0; --ci)
				{
					m_ar[ ri][ ci] = (float)(ci * ci)/(float)(ri * ri);
				}
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

	return (wi >= m_minClusterSize) ? ((1.0 - m_config.minFfWeight) * (ww/4) + m_config.minFfWeight) : 0.0;
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
		if (ti != ni->featidx && m_postings[ ti])
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
#ifdef STRUS_LOWLEVEL_DEBUG
		// Some validity check when debugging:
		if (ni->touchCount() > (m_nofPostings - (m_hasPunctuation ? 1 : 0)))
		{
			throw std::runtime_error(_TXT("logic error: proximity weighting touch count exceeds number of features"));
		}
#endif
	}
}

void ProximityWeightingContext::initStructures( StructIteratorInterface* structIterator, strus::Index structno)
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
		if (tc >= m_config.minClusterSize)
		{
			++nofCandidates[ tc];
		}
	}
	{
		int sum = 0;
		int tc = m_nofPostings;
		for (;tc >= m_config.minClusterSize;--tc)
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
									if (!structno || structno == lnk.structno())
									{
										fieldUsed = true;
										m_stmOperations.push_back( StmOperation( field.start(), m_fieldStatistics.size()));
										m_stmOperations.push_back( StmOperation( field.start(), field.end(), m_fieldStatistics.size()));
										m_fieldStatistics.push_back( FieldStatistics( field));
									}
								}
								StructIteratorInterface::HeaderField hh = structIterator->headerField( lnk.index());
								touchTitleNode( ni, hh.field(), field);
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
		m_fieldStatistics.push_back( FieldStatistics());
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
		FieldStatistics& res = m_fieldStatistics.back();

		std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
		for (; ni != ne; ++ni)
		{
			if (ni->featidx != eos_featidx)
			{
				res.ff[ ni->featidx-ofs_featidx] += ff_weight( *ni);
			}
		}
	}
	else
	{
		FieldStatistics& totalStats = m_fieldStatistics.back();
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
					totalStats.ff[ ni->featidx - ofs_featidx] += ww;
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

struct WeightedPos
{
	double weight;
	strus::Index pos;
	strus::Index sent_idx;
};

struct WeightedPosWindow
{
private:
	enum {
		MaxSummaryFieldSize = ProximityWeightingContext::MaxSummaryFieldSize,
		MaxSummaryFieldMask = MaxSummaryFieldSize-1
	};

	WeightedPos weightedPos[ MaxSummaryFieldSize];
	strus::Index sentenceStart[ MaxSummaryFieldSize];
	int nofPositions;
	int cur_idx;
	int start_idx;
	int sent_idx;
	double weightsum;

public:
	explicit WeightedPosWindow( int windowSize)
		:nofPositions(windowSize),cur_idx(0),start_idx(0),sent_idx( 0),weightsum(0.0)

	{
		STRUS_STATIC_ASSERT( MaxSummaryFieldSize != 0
				&& (MaxSummaryFieldSize & (MaxSummaryFieldSize-1)) == 0);

		if (nofPositions >= MaxSummaryFieldSize) nofPositions = MaxSummaryFieldSize-1;

		std::memset( &weightedPos, 0, sizeof(weightedPos));
		std::memset( &sentenceStart, 0, sizeof(sentenceStart));
	}

	void pushEos( strus::Index pos)
	{
		++sent_idx;
		sentenceStart[ sent_idx & MaxSummaryFieldMask] = pos;
	}

	void pushWeight( strus::Index pos, double weight)
	{
		cur_idx = (cur_idx + 1) & MaxSummaryFieldMask;
		WeightedPos& cur = weightedPos[ cur_idx];
		cur.pos = pos;
		cur.sent_idx = sent_idx;
		cur.weight = weight;
	}

	void calculateWeightSum()
	{
		WeightedPos& cur = weightedPos[ cur_idx];
		weightsum += cur.weight;
		while (start_idx != cur_idx && weightedPos[ start_idx].pos + nofPositions < cur.pos)
		{
			weightsum -= weightedPos[ start_idx].weight;
			start_idx = (start_idx + 1) & MaxSummaryFieldMask;
		}
	}

	strus::IndexRange getCurrentSentences( int nofSentences, strus::Index nextSentenceStart, int dist)
	{
		strus::IndexRange rt;
		int sent_idx_start = weightedPos[ cur_idx].sent_idx - nofSentences;
		if (sent_idx_start < 0) sent_idx_start = 0;
		strus::Index start = sentenceStart[ sent_idx_start & MaxSummaryFieldMask];
		strus::Index end = weightedPos[ cur_idx].pos + dist;
		while (sent_idx_start != weightedPos[ cur_idx].sent_idx && start + nofPositions < end)
		{
			++sent_idx_start;
			start = sentenceStart[ sent_idx_start & MaxSummaryFieldMask];
		}
		if (start + nofPositions < end)
		{
			start = end - nofPositions + dist;
			if (start >= end) start = end -1;
		}
		if (nextSentenceStart <= start + nofPositions)
		{
			end = nextSentenceStart;
		}
		rt.init( start, end);
		return rt;
	}

	strus::IndexRange getCurrentField() const
	{
		return strus::IndexRange( weightedPos[ start_idx].pos, weightedPos[ cur_idx].pos+1);
	}

	const WeightedPos& currentWeightedPos() const
	{
		return weightedPos[ cur_idx];
	}

	double currentWeightSum() const
	{
		return weightsum;
	}

	strus::Index currentSentenceStart() const
	{
		return sentenceStart[ sent_idx & MaxSummaryFieldMask];
	}
};

strus::IndexRange ProximityWeightingContext::getBestPassage( const FeatureWeights& featureWeights) const
{
	strus::IndexRange rt;

	double weightsum_max = 0.0;
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	WeightedPosWindow window( m_config.nofSummarySentences * m_config.maxNofSummarySentenceWords);

	std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
	while (ni != ne)
	{
		if (ni->featidx == eos_featidx)
		{
			window.pushEos( ni->pos);
			++ni;
			continue;
		}
		strus::Index curpos = window.currentWeightedPos().pos;
		double curweight = 0.0;
		for (; ni != ne && ni->pos == curpos; ++ni)
		{
			curweight += featureWeights[ ni->featidx] * ff_weight( *ni);
		}
		window.pushWeight( curpos, curweight);
		window.calculateWeightSum();

		if (window.currentWeightSum() > weightsum_max)
		{
			weightsum_max = window.currentWeightSum();
			if (m_hasPunctuation)
			{
				strus::Index nextSentenceStart = ni->pos+1;
				std::vector<Node>::const_iterator next_ni = ni;
				for (++next_ni;
					next_ni != ne
					&& next_ni->featidx != eos_featidx
					&& next_ni->pos - ni->pos <= m_config.maxNofSummarySentenceWords;
					++next_ni){}
				if (next_ni != ne && next_ni->featidx != eos_featidx)
				{
					nextSentenceStart = next_ni->pos;
				}
				rt = window.getCurrentSentences( m_config.nofSummarySentences, nextSentenceStart, m_config.distance_close);
			}
			else
			{
				rt = window.getCurrentField();
			}
		}
	}
	return rt;
}

static void addWeightedNeighbourWeights( std::vector<ProximityWeightingContext::WeightedNeighbour>& res, strus::Index startpos, strus::Index endpos, double weight)
{
	if (!res.empty())
	{
		std::vector<ProximityWeightingContext::WeightedNeighbour>::iterator ri = res.end(), rb = res.begin();
		for (; ri > rb && ri->pos >= startpos; --ri)
		{
			ri->weight += weight;
		}
		startpos = res.back().pos + 1;
	}
	strus::Index pos = startpos;
	for (; pos < endpos; ++pos)
	{
		res.push_back( ProximityWeightingContext::WeightedNeighbour( pos, weight));
	}
}

void ProximityWeightingContext::collectWeightedNeighbours( std::vector<WeightedNeighbour>& res, const FeatureWeights& featureWeights, strus::Index dist) const
{
	res.clear();

	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	WeightedPosWindow window( m_config.nofSummarySentences * m_config.maxNofSummarySentenceWords);

	std::vector<Node>::const_iterator ni = m_nodear.begin(), ne = m_nodear.end();
	while (ni != ne)
	{
		
		if (ni->featidx == eos_featidx)
		{
			window.pushEos( ni->pos);
			std::vector<WeightedNeighbour>::iterator ri = res.end(), rb = res.begin();
			for (; ri > rb && ri->pos >= ni->pos; --ri){}
			res.resize( ri-rb);
			++ni;
			continue;
		}
		strus::Index curpos = window.currentWeightedPos().pos;
		double curweight = 0.0;
		int max_tc = 0;
		for (; ni != ne && ni->pos == curpos; ++ni)
		{
			int tc = ni->touchCount();
			if (tc > max_tc) max_tc = tc;
			curweight += featureWeights[ ni->featidx] * ff_weight( *ni);
		}
		if (curweight <= std::numeric_limits<double>::min()) continue;

		window.pushWeight( curpos, curweight);
		const WeightedPos& match = window.currentWeightedPos();

		strus::Index startpos = match.pos > dist ? match.pos - dist : 1;
		strus::Index endpos = match.pos + dist + m_length_postings[ ni->featidx];
		if (startpos <= window.currentSentenceStart())
		{
			startpos = window.currentSentenceStart()+1;
		}
		addWeightedNeighbourWeights( res, startpos, endpos, match.weight);
	}
}

void ProximityWeightingContext::collectWeightedNeighboursForSingleFeature( 
		std::vector<WeightedNeighbour>& res,
		strus::Index dist,
		PostingIteratorInterface* postings,
		PostingIteratorInterface* eos_postings, 
		strus::Index docno,
		const strus::IndexRange& field)
{
	res.clear();

	bool hasPunctuation = (eos_postings != NULL && eos_postings->skipDoc(docno) == docno);
	if (postings->skipDoc(docno) != docno) return;

	strus::Index pos = postings->skipPos( field.start());
	strus::Index endpos = field.defined() ? field.end() : (strus::Index)std::numeric_limits<Position>::max();
	strus::Index featlen = postings->length();

	if (hasPunctuation)
	{
		for (; pos && pos < endpos; pos = postings->skipPos( pos+1))
		{
			strus::Index rangeStart = pos-dist;
			strus::Index rn = eos_postings->skipPos( rangeStart);
			for (; rn && rn < pos; rn = eos_postings->skipPos( rn+1))
			{
				rangeStart = rn;
			}
			strus::Index rangeEnd = eos_postings->skipPos( pos);
			if (rangeEnd > pos + dist)
			{
				rangeEnd = pos + dist + featlen;
			}
			addWeightedNeighbourWeights( res, rangeStart, rangeEnd, 1.0);
		}
	}
	else
	{
		for (; pos && pos < endpos; pos = postings->skipPos( pos+1))
		{
			strus::Index rangeStart = pos > dist ? pos - dist : 1;
			strus::Index rangeEnd = pos + featlen + dist;
			addWeightedNeighbourWeights( res, rangeStart, rangeEnd, 1.0);
		}
	}
}

void ProximityWeightingContext::Config::setDistanceImm( int distance_imm_)
{
	if (distance_imm_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
	distance_imm = distance_imm_;
}
void ProximityWeightingContext::Config::setDistanceClose( int distance_close_)
{
	if (distance_close_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
	distance_close = distance_close_;
}
void ProximityWeightingContext::Config::setDistanceNear( int distance_near_)
{
	if (distance_near_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
	distance_near = distance_near_;
}
void ProximityWeightingContext::Config::setMinClusterSize( float minClusterSize_)
{
	if (minClusterSize_ <= 0.0 || minClusterSize_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
	minClusterSize = minClusterSize_;
}
void ProximityWeightingContext::Config::setNofHotspots( float nofHotspots_)
{
	if (nofHotspots_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	nofHotspots = nofHotspots_;
}
void ProximityWeightingContext::Config::setNofSummarySentences( float nofSummarySentences_)
{
	if (nofSummarySentences_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	if (nofSummarySentences_ >= MaxSummaryFieldSize) throw std::runtime_error(_TXT("value out of range"));
	nofSummarySentences = nofSummarySentences_;
}
void ProximityWeightingContext::Config::setMaxNofSummarySentenceWords( float maxNofSummarySentenceWords_)
{
	if (maxNofSummarySentenceWords_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	maxNofSummarySentenceWords = maxNofSummarySentenceWords_;
}
void ProximityWeightingContext::Config::setMinFfWeight( double minFfWeight_)
{
	if (minFfWeight_ < 0.0 || minFfWeight_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
	minFfWeight = minFfWeight_;
}
