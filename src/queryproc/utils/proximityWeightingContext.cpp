/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "proximityWeightingContext.hpp"
#include "forwardIndexCollector.hpp"
#include "strus/structureIteratorInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/static_assert.hpp"
#include "strus/structureHeaderField.hpp"
#include <limits>
#include <set>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG_FIELD //... on enabling assign a tuple like 121,127
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
	strus::Index endpos = m_field.defined() ? m_field.end() : std::numeric_limits<strus::Index>::max();
	strus::Index posnoar[ MaxNofArguments];			//... array parallel to m_postings, m_featureids and m_length_postings

	if (true == (m_hasPunctuation = (eos_postings != NULL && eos_postings->skipDoc(docno) == docno)))
	{
		eos_featidx = 0;
		m_length_postings[ m_nofPostings] = eos_postings->length();
		m_postings[ m_nofPostings] = eos_postings;
		m_featureids[ m_nofPostings] = 0;
		featureIdToStartMap[ m_nofPostings] = field.start();
		featureIdToNameMap[ m_nofPostings] = 0;

		posnoar[ m_nofPostings] = m_postings[ m_nofPostings]->skipPos( field.start());
		if (0 == posnoar[ m_nofPostings] || posnoar[ m_nofPostings] > endpos)
		{
			posnoar[ m_nofPostings] = endpos;
			featureIdToStartMap[ m_nofPostings] = endpos;
		}
		else
		{
			featureIdToStartMap[ m_nofPostings] = posnoar[ m_nofPostings]+1;
		}
		++m_nofPostings;
		firstPostingIdx = m_nofPostings;
	}
	m_nodear.clear();
	m_stmOperations.clear();
	m_fieldStatistics.clear();
	m_stmStack.clear();
	m_nodeScanner.init( 0, 0);

	// [2] Inialize features for the sliding window scan with a queue:
	unsigned char queue[ MaxNofArguments];	//... array with indices into m_postings and posnoar
	int pi=0;
	for (;pi<nofPostings; ++pi)
	{
		m_postings[ m_nofPostings] = postings[ pi];
		m_length_postings[ m_nofPostings] = m_postings[ m_nofPostings]->length();
		m_featureids[ m_nofPostings] = m_nofPostings;
		featureIdToNameMap[ m_nofPostings] = m_postings[ m_nofPostings]->featureid();
		featureIdToStartMap[ m_nofPostings] = field.start();

		int firstAppearanceIndex = indexStringArray( featureIdToNameMap, m_nofPostings, firstPostingIdx, featureIdToNameMap[ m_nofPostings]);
		int featid = m_featureids[ m_nofPostings] = m_featureids[ firstAppearanceIndex];

		if (m_postings[ m_nofPostings]->skipDoc(docno) == docno)
		{
			posnoar[ m_nofPostings]
				= m_postings[ m_nofPostings]->skipPos( featureIdToStartMap[ featid]);
			if (0 == posnoar[ m_nofPostings] || posnoar[ m_nofPostings] > endpos)
			{
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
			posnoar[ m_nofPostings] = 0;
			featureIdToStartMap[ featid] = endpos;
		}
		++m_nofPostings;
	}
	// [3] Initialize the queue for the sliding window scan:
	int queueSize = 0;
	for (pi=0; pi<m_nofPostings; ++pi)
	{
		if (posnoar[ pi])
		{
			int ki = 0, ke = queueSize;
			for (; ki < ke && posnoar[ queue[ ki]] < posnoar[ pi]; ++ki){}
			if (ki < pi)
			{
				// Move bigger or equal elements one position higher in the queue
				//	to create a slot for the new element inserted into the queue:
				for (; ke > ki; --ke)
				{
					queue[ ke] = queue[ ke-1];
				}
			}
			// Insert at the end or in the slot freed before:
			queue[ ki] = pi;
			++queueSize;
		}
	}
	// [4] Generate node array with sliding window scan:
	int min_queueSize = m_minClusterSize + (m_hasPunctuation ? 1 : 0);
	while (queueSize >= min_queueSize)
	{
		// Take first feature from queue and reinsert it:
		int fidx = queue[ 0];
		strus::Index documentPos = posnoar[ fidx];
		if (documentPos < endpos)
		{
			if (fidx == eos_featidx && !m_nodear.empty() && m_nodear.back().featidx == eos_featidx)
			{
				//... do not add subsequent eos markers as they are only used as delimiters
			}
			else
			{
				m_nodear.push_back( Node( fidx, documentPos));
			}
			// Get the next start position of the top element in the queue
			strus::Index nextPos = featureIdToStartMap[ m_featureids[ fidx]];
			strus::Index jumpPos = posnoar[ queue[ (fidx == eos_featidx) ? 1 : min_queueSize-1 ]];
			if (nextPos < jumpPos - m_config.distance_near)
			{
				nextPos = jumpPos - m_config.distance_near;
			}
			if (nextPos <= documentPos)
			{
				throw std::runtime_error(_TXT("logic error: proximity weighting queue corrupted"));
			}
			posnoar[ fidx] = m_postings[ fidx]->skipPos( nextPos);
		}
		else
		{
			posnoar[ fidx] = 0;
		}
		if (posnoar[ fidx] == 0 || posnoar[ fidx] >= endpos)
		{
			// Set start position at end of all:
			featureIdToStartMap[ m_featureids[ fidx]] = endpos;

			// Delete this element from queue by moving all elements
			//	to the left eliminating the first slot:
			std::memmove( queue, queue+1, queueSize-1);
			queueSize--;
		}
		else
		{
			// Set new start position of this feature:
			featureIdToStartMap[ m_featureids[ fidx]] = posnoar[ fidx]+1;

			// Move all smaller elemnts towards the start of the queue overwriting the first
			//	slot. A new slot is created at the position ki-1 in this process:
			int ki = 1;
			for (; ki<queueSize && posnoar[ queue[ ki]] < posnoar[ fidx]; ++ki)
			{
				queue[ ki-1] = queue[ ki];
			}
			// Reinsert in queue at the created slot, keeping the number of elements
			//	in the queue the same:
			queue[ ki-1] = fidx;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		{
			// Verify validity in debug mode with real sets:
			std::set<int> check_queue_set;
			for (int ki=0; ki<queueSize; ++ki)
			{
				if (ki && posnoar[ queue[ ki-1] ] > posnoar[ queue[ ki] ])
				{
					throw std::runtime_error(_TXT("logic error: queue corrupted in proximity weighting (element order)"));
				}
				check_queue_set.insert( queue[ ki]);
			}
			if ((int)check_queue_set.size() != queueSize)
			{
				throw std::runtime_error(_TXT("logic error: queue corrupted in proximity weighting (duplicates)"));
			}
		}
#endif
	}
	initNeighbourMatches();
	m_nodeScanner.init( m_nodear.data(), m_nodear.size());
}

void ProximityWeightingContext::validateTouchCount()
{
	// Some validity verification when debugging:
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		if (ni->touchCount() > m_nofWeightedNeighbours)
		{
			throw std::runtime_error(_TXT("logic error: proximity weighting touch count exceeds number of weighted peer relations"));
		}
	}
}

void ProximityWeightingContext::markTouches( Node::TouchType touchType, std::vector<Node>::iterator ni, std::vector<Node>::iterator ne, strus::Index dist)
{
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	strus::Index ni_endpos = ni->pos + m_length_postings[ ni->featidx];
	std::vector<Node>::iterator fi = ni+1, fe = ne;
	for (; fi != fe && ni_endpos + dist > fi->pos; ++fi)
	{
		if (fi->featidx != eos_featidx && fi->featidx != ni->featidx)
		{
			if (fi->touched.set( ni->featidx, true))
			{
				fi->incrementTouchCount( touchType);
			}
			if (ni->touched.set( fi->featidx, true))
			{
				ni->incrementTouchCount( touchType);
			}
		}
	}
}

void ProximityWeightingContext::markTouchesInSentence( Node::TouchType touchType, std::vector<Node>::iterator ni, std::vector<Node>::iterator ne, strus::Index dist)
{
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	strus::Index ni_endpos = ni->pos + m_length_postings[ ni->featidx];
	std::vector<Node>::iterator fi = ni+1, fe = ne;
	for (; fi != fe && fi->featidx != eos_featidx && ni_endpos + dist > fi->pos; ++fi)
	{
		if (fi->featidx != ni->featidx)
		{
			if (fi->touched.set( ni->featidx, true))
			{
				fi->incrementTouchCount( touchType);
			}
			if (ni->touched.set( fi->featidx, true))
			{
				ni->incrementTouchCount( touchType);
			}
		}
	}
}

void ProximityWeightingContext::initNeighbourMatches()
{
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	if (m_hasPunctuation)
	{
		// Mark relations (imm,close,sentence,near):
		for (; ni != ne; ++ni)
		{
			if (ni->featidx != 0/*EOS*/)
			{
				markTouchesInSentence( Node::ImmediateTouch, ni, ne, m_config.distance_imm);
			}
		}
		for (ni = m_nodear.begin(); ni != ne; ++ni)
		{
			if (ni->featidx != 0/*EOS*/)
			{
				markTouchesInSentence( Node::CloseTouch, ni, ne, m_config.distance_close);
			}
		}
		for (ni = m_nodear.begin(); ni != ne; ++ni)
		{
			if (ni->featidx != 0/*EOS*/)
			{
				markTouchesInSentence( Node::SentenceTouch, ni, ne, m_config.maxNofSummarySentenceWords);
			}
		}
		for (ni = m_nodear.begin(); ni != ne; ++ni)
		{
			if (ni->featidx != 0/*EOS*/)
			{
				markTouches( Node::NearTouch, ni, ne, m_config.distance_near);
			}
		}
	}
	else
	{
		// Mark relations (imm,close,near):
		for (; ni != ne; ++ni)
		{
			markTouches( Node::ImmediateTouch, ni, ne, m_config.distance_imm);
		}
		for (ni = m_nodear.begin(); ni != ne; ++ni)
		{
			markTouches( Node::CloseTouch, ni, ne, m_config.distance_close);
		}
		for (ni = m_nodear.begin(); ni != ne; ++ni)
		{
			markTouches( Node::NearTouch, ni, ne, m_config.distance_near);
		}
	}
#ifdef STRUS_LOWLEVEL_DEBUG
	validateTouchCount();
#endif
	// Eliminate nodes with a touch count lower than the minimum window size:
	ni = m_nodear.begin();
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

double ProximityWeightingContext::ff_weight( const Node& nd) const
{
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
	for (; start >= m_nodear.begin() && start->pos >= contentField.start(); --start){}
	++start;
	std::vector<Node>::iterator end = m_nodear.end();
	int nextidx = m_nodeScanner.upperbound( contentField.end(), ni-m_nodear.begin(), m_nodear.size());
	if (nextidx > 0)
	{
		end = m_nodear.begin() + nextidx;
	}
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
		if (ni->touched.set( ni->featidx, false)) {--tc;} //... unmask own references that are not counted
#ifdef STRUS_LOWLEVEL_DEBUG
		if (tc < 0) throw std::runtime_error(_TXT("logic error: proximity weighting counting corrupted (title matches)"));
#endif
		ni->titleScopeMatches += tc;
	}
}

void ProximityWeightingContext::initStructures( StructureIteratorInterface* structIterator, strus::Index structno)
{
	m_stmOperations.clear();
	m_fieldStatistics.clear();
	m_stmStack.clear();
	int nofCandidates[MaxNofArguments];
	std::memset( nofCandidates, 0, sizeof( nofCandidates));
	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;

	structIterator->skipDoc( m_docno);
	std::vector<Node>::iterator ni = m_nodear.begin(), ne = m_nodear.end();
	for (; ni != ne; ++ni)
	{
		int tc = ni->touchCount();
		if (tc >= m_minClusterSize)
		{
			++nofCandidates[ tc];
		}
	}
	{
		int sum = 0;
		int tc = m_nofPostings;
		for (;tc >= m_minClusterSize;--tc)
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
				if (ni->featidx != eos_featidx && ni->touchCount() >= tc)
				{
					strus::IndexRange field = structIterator->skipPos( li, ni->pos);
					if (!field.defined()) break;
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
								StructureHeaderField hh = structIterator->headerField( lnk.index());
								touchTitleNode( ni, hh.field(), field);
							}
						}
						int nextidx = m_nodeScanner.upperbound( field.end(), ni-m_nodear.begin(), m_nodear.size());
						if (nextidx > 0)
						{
							ni = m_nodear.begin() + nextidx;
						}
						else
						{
							ni = ne;
						}
					}
					else
					{
						int nextidx = m_nodeScanner.upperbound( field.start(), ni-m_nodear.begin(), m_nodear.size());
						if (nextidx > 0)
						{
							ni = m_nodear.begin() + nextidx;
						}
						else
						{
#ifdef STRUS_LOWLEVEL_DEBUG
							if (nextidx == 0) throw std::runtime_error(_TXT("logic error: upperbound seek returns element that failed condition before"));
#endif
							ni = ne;
						}
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
#ifdef STRUS_LOWLEVEL_DEBUG
	validateTouchCount();
#endif
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
				res.ff[ ni->featidx - ofs_featidx] += ff_weight( *ni);
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
				// Structure operation on the stack of currently open fields:
				if (oi->isStartTag())
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
			for (; ni != ne && oi != oe && ni->pos < oi->endpos; ++ni)
			{
				if (ni->featidx != eos_featidx)
				{
					// Accumulate weights for the open fields:
					double ww = ff_weight( *ni);
	
					std::vector<int>::const_iterator
						ki = m_stmStack.begin(), ke = m_stmStack.end();
					for (; ki != ke; ++ki)
					{
						m_fieldStatistics[ *ki].ff[ ni->featidx - ofs_featidx] += ww;
					}
#ifdef STRUS_LOWLEVEL_DEBUG_FIELD
					if (strus::IndexRange(STRUS_LOWLEVEL_DEBUG_FIELD).contain( ni->pos))
					{
						std::cerr << strus::string_format( "DEBUG real weighting pos=%d, qryidx=%d, ff=%.8f, touch: {imm=%d, close=%d, sent=%d, near=%d, title=%d}", (int)ni->pos, (int)(ni->featidx - ofs_featidx), ww, (int)ni->immediateMatches, (int)ni->closeMatches, (int)ni->sentenceMatches, (int)ni->nearMatches, (int)ni->titleScopeMatches) << std::endl;
					}
#endif
					// Accumulate weights for the total stats:
					totalStats.ff[ ni->featidx - ofs_featidx] += ww;
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		// Check stack operations left to be balanced
		for (; oi != oe; ++oi)
		{
			//... structure operations on the stack of currently open fields
			if (oi->isStartTag())
			{
				m_stmStack.push_back( oi->fieldidx);
			}
			else if (!m_stmStack.empty() && oi->fieldidx == m_stmStack.back())
			{
				m_stmStack.pop_back();
			}
			else
			{
				throw std::runtime_error(_TXT("logic error: unbalanced open/close on field stack (final check)"));
			}
		}
#endif
		// Accumulate weights not covered by any structure for the total stats:
		for (; ni != ne; ++ni)
		{
			if (ni->featidx != eos_featidx)
			{
				double ww = ff_weight( *ni);
				totalStats.ff[ ni->featidx - ofs_featidx] += ww;
			}
		}
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
	int ofs_featidx = m_hasPunctuation ? 1 : 0;

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
		strus::Index curpos = ni->pos;
		double curweight = 0.0;
		bool gotEos = false;
		for (; ni != ne && ni->pos == curpos; ++ni)
		{
			if (ni->featidx != eos_featidx)
			{
				curweight += featureWeights[ ni->featidx - ofs_featidx] * ff_weight( *ni);
			}
			else
			{
				gotEos = true;
			}
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
		if (gotEos)
		{
			window.pushEos( ni->pos);
			++ni;
		}
	}
	return rt;
}

static void addWeightedNeighbourWeights(
		std::vector<ProximityWeightingContext::WeightedNeighbour>& res,
		strus::Index startpos, strus::Index endpos,
		double weight)
{
	if (!res.empty())
	{
		std::vector<ProximityWeightingContext::WeightedNeighbour>::iterator ri = res.end(), rb = res.begin();
		for (--ri; ri >= rb && ri->pos >= startpos; --ri)
		{
			ri->weight += weight;
		}
		if (startpos <= res.back().pos)
		{
			startpos = res.back().pos + 1;
		}
	}
	strus::Index pos = startpos;
	for (; pos < endpos; ++pos)
	{
		res.push_back( ProximityWeightingContext::WeightedNeighbour( pos, weight));
	}
}

std::vector<ProximityWeightingContext::WeightedNeighbour>
	ProximityWeightingContext::getWeightedNeighbours(
		const FeatureWeights& featureWeights,
		strus::Index dist) const
{
	std::vector<ProximityWeightingContext::WeightedNeighbour> rt;

	unsigned char eos_featidx = m_hasPunctuation ? 0 : 0xFF;
	int ofs_featidx = m_hasPunctuation ? 1 : 0;

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
		strus::Index curpos = ni->pos;
		double curweight = 0.0;
		int max_tc = 0;
		bool gotEos = false;
		for (; ni != ne && ni->pos == curpos; ++ni)
		{
			if (ni->featidx != eos_featidx)
			{
				int tc = ni->touchCount();
				if (tc > max_tc) max_tc = tc;
				curweight += featureWeights[ ni->featidx - ofs_featidx] * ff_weight( *ni);
			}
			else
			{
				gotEos = true;
			}
		}
		if (curweight <= std::numeric_limits<double>::min()) continue;

		window.pushWeight( curpos, curweight);
		const WeightedPos& match = window.currentWeightedPos();

		strus::Index startpos = match.pos > dist ? match.pos - dist : 1;
		strus::Index endpos = match.pos + dist + 1;
		if (startpos <= window.currentSentenceStart())
		{
			startpos = window.currentSentenceStart()+1;
		}
		addWeightedNeighbourWeights( rt, startpos, endpos, match.weight);
		if (gotEos)
		{
			window.pushEos( ni->pos);
			++ni;
		}
	}
	return rt;
}

std::vector<ProximityWeightingContext::WeightedNeighbour>
	ProximityWeightingContext::getWeightedNeighboursForSingleFeature( 
		strus::Index dist,
		PostingIteratorInterface* postings,
		PostingIteratorInterface* eos_postings, 
		double featureWeight,
		strus::Index docno,
		const strus::IndexRange& field)
{
	std::vector<ProximityWeightingContext::WeightedNeighbour> rt;

	bool hasPunctuation = (eos_postings != NULL && eos_postings->skipDoc(docno) == docno);
	if (postings->skipDoc(docno) != docno) return rt;

	strus::Index pos = postings->skipPos( field.start());
	strus::Index endpos = field.defined() ? field.end() : std::numeric_limits<strus::Index>::max();
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
			addWeightedNeighbourWeights( rt, rangeStart, rangeEnd, featureWeight);
		}
	}
	else
	{
		for (; pos && pos < endpos; pos = postings->skipPos( pos+1))
		{
			strus::Index rangeStart = pos > dist ? pos - dist : 1;
			strus::Index rangeEnd = pos + featlen + dist;
			addWeightedNeighbourWeights( rt, rangeStart, rangeEnd, featureWeight);
		}
	}
	return rt;
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
void ProximityWeightingContext::Config::setMinClusterSize( double minClusterSize_)
{
	if (minClusterSize_ < 0.0 || minClusterSize_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
	minClusterSize = minClusterSize_;
}
void ProximityWeightingContext::Config::setNofHotspots( int nofHotspots_)
{
	if (nofHotspots_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	nofHotspots = nofHotspots_;
}
void ProximityWeightingContext::Config::setNofSummarySentences( int nofSummarySentences_)
{
	if (nofSummarySentences_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	if (nofSummarySentences_ >= MaxSummaryFieldSize) throw std::runtime_error(_TXT("value out of range"));
	nofSummarySentences = nofSummarySentences_;
}
void ProximityWeightingContext::Config::setMaxNofSummarySentenceWords( int maxNofSummarySentenceWords_)
{
	if (maxNofSummarySentenceWords_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
	maxNofSummarySentenceWords = maxNofSummarySentenceWords_;
}
void ProximityWeightingContext::Config::setMinFfWeight( double minFfWeight_)
{
	if (minFfWeight_ < 0.0 || minFfWeight_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
	minFfWeight = minFfWeight_;
}
