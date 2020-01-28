/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_PROXIMITY_WEIGHTING_CONTEXT_HPP_INCLUDED
#define _STRUS_QUERYPROC_PROXIMITY_WEIGHTING_CONTEXT_HPP_INCLUDED
#include "strus/base/bitset.hpp"
#include "strus/index.hpp"
#include "private/internationalization.hpp"
#include <limits>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class StructIteratorInterface;

class ProximityWeightingContext
{
public:
	enum {MaxNofArguments=64};
	struct Config
	{
		int distance_imm;
		int distance_close;
		int distance_near;
		int minClusterSize;
		int nofHotspots;

		Config( int distance_imm_, int distance_close_, int distance_near_, int minClusterSize_)
			:distance_imm(distance_imm_)
			,distance_close(distance_close_)
			,distance_near(distance_near_)
			,minClusterSize(minClusterSize_){}
		Config( const Config& o)
			:distance_imm(o.distance_imm)
			,distance_close(o.distance_close)
			,distance_near(o.distance_near)
			,minClusterSize(o.minClusterSize){}
	};

	struct Node
	{
		strus::bitset<MaxNofArguments> touched;
		unsigned short pos;
		unsigned char closeMatches;
		unsigned char immFollowMatches;
		unsigned char sentenceMatches;
		unsigned char nearMatches;
		unsigned char titleScopeMatches;
		unsigned char featidx;

		Node( int featidx_, strus::Index documentPos)
			:touched()
			,pos(documentPos)
			,closeMatches(0)
			,immFollowMatches(0)
			,sentenceMatches(0)
			,nearMatches(0)
			,titleScopeMatches(0)
			,featidx(featidx_)
		{
			if (featidx_ < 0 || featidx_ >= MaxNofArguments)
			{
				throw std::runtime_error(_TXT("number of features out of range"));
			}
			if (documentPos < 0 || documentPos >= std::numeric_limits<unsigned short>::max())
			{
				throw std::runtime_error(_TXT("position out of range"));
			}
		}
		Node( const Node& o)
			:touched(o.touched)
			,pos(o.pos)
			,closeMatches(o.closeMatches)
			,immFollowMatches(o.immFollowMatches)
			,sentenceMatches(o.sentenceMatches)
			,nearMatches(o.nearMatches)
			,titleScopeMatches(o.titleScopeMatches)
			,featidx(o.featidx){}
	};

public:
	ProximityWeightingContext( const ProximityWeightingContext& o)
		:m_nodear(o.m_nodear){}
	ProximityWeightingContext()
		:m_nodear(){}
	
	void init(
		PostingIteratorInterface** postings, int nofPostings, const Config& config,
		PostingIteratorInterface* eos_postings, strus::Index docno);

	void cluster( const Config& config);

private:
	std::vector<Node> m_nodear;
};

