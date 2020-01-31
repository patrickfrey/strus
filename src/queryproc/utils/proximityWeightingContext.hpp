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
#include "private/skipScanArray.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <cstring>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class StructIteratorInterface;

class ProximityWeightingContext
{
public:
	enum {
		MaxNofArguments=64,
		MaxSummaryFieldSize=512
	};
	struct Config
	{
		int distance_imm;
		int distance_close;
		int distance_near;
		float minClusterSize;
		int nofHotspots;
		int nofSentencesSummary;
		int maxNofSentenceWords;
		double minFfWeight;
		strus::Index structno;

		Config()
			:distance_imm(2)
			,distance_close(8)
			,distance_near(40)
			,minClusterSize(0.7)
			,nofHotspots(10)
			,nofSentencesSummary(2)
			,maxNofSentenceWords(20)
			,minFfWeight(0.1)
			,structno(0){}
		Config( const Config& o)
			:distance_imm(o.distance_imm)
			,distance_close(o.distance_close)
			,distance_near(o.distance_near)
			,minClusterSize(o.minClusterSize)
			,nofHotspots(o.nofHotspots)
			,nofSentencesSummary(o.nofSentencesSummary)
			,maxNofSentenceWords(o.maxNofSentenceWords)
			,minFfWeight(o.minFfWeight)
			,structno(o.structno){}

		void setDistanceImm( int distance_imm_)
		{
			if (distance_imm_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
			distance_imm = distance_imm_;
		}
		void setDistanceClose( int distance_close_)
		{
			if (distance_close_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
			distance_close = distance_close_;
		}
		void setDistanceNear( int distance_near_)
		{
			if (distance_near_ < 0) throw std::runtime_error(_TXT("expected positive integer value"));
			distance_near = distance_near_;
		}
		void setMinClusterSize( float minClusterSize_)
		{
			if (minClusterSize_ <= 0.0 || minClusterSize_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
			minClusterSize = minClusterSize_;
		}
		void setNofHotspots( float nofHotspots_)
		{
			if (nofHotspots_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
			nofHotspots = nofHotspots_;
		}
		void setNofSentencesSummary( float nofSentencesSummary_)
		{
			if (nofSentencesSummary_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
			nofSentencesSummary = nofSentencesSummary_;
		}
		void setMaxNofSentenceWords( float maxNofSentenceWords_)
		{
			if (maxNofSentenceWords_ <= 0) throw std::runtime_error(_TXT("expected positive integer value"));
			maxNofSentenceWords = maxNofSentenceWords_;
		}
		void setMinFfWeight( double minFfWeight_)
		{
			if (minFfWeight_ < 0.0 || minFfWeight_ > 1.0) throw std::runtime_error(_TXT("value out of range [0.0,1.0]"));
			minFfWeight = minFfWeight_;
		}
		void setContentStructure( strus::Index structno_)
		{
			if (structno_ < 0) throw std::runtime_error(_TXT("expected valid structure identifier"));
			structno = structno_;
		}
	};

	typedef unsigned short Position;

	struct Node
	{
		strus::bitset<MaxNofArguments> touched;
		Position pos;
		unsigned char closeMatches;
		unsigned char immediateMatches;
		unsigned char sentenceMatches;
		unsigned char nearMatches;
		unsigned char titleScopeMatches;
		unsigned char featidx;

		Node( int featidx_, Position documentPos)
			:touched()
			,pos(documentPos)
			,closeMatches(0)
			,immediateMatches(0)
			,sentenceMatches(0)
			,nearMatches(0)
			,titleScopeMatches(0)
			,featidx(featidx_)
		{}
		Node( const Node& o)
			:touched(o.touched)
			,pos(o.pos)
			,closeMatches(o.closeMatches)
			,immediateMatches(o.immediateMatches)
			,sentenceMatches(o.sentenceMatches)
			,nearMatches(o.nearMatches)
			,titleScopeMatches(o.titleScopeMatches)
			,featidx(o.featidx){}

		int inline touchCount() const
		{
			return closeMatches + immediateMatches + sentenceMatches + nearMatches;
		}
		/// \brief Comparator functor to find nodes with a position greator or equal to needle
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const Node& aa, Position bb) const
			{
				return aa.pos < bb;
			}
		};
	};

	typedef SkipScanArray<Node,Position,Node::SearchCompare> NodeScanner;

public:
	ProximityWeightingContext( const ProximityWeightingContext& o)
		:m_config(o.m_config)
		,m_nofPostings(o.m_nofPostings),m_nofWeightedNeighbours(o.m_nofWeightedNeighbours)
		,m_hasPunctuation(o.m_hasPunctuation),m_docno(o.m_docno),m_field(o.m_field)
		,m_nodear(o.m_nodear)
		,m_stmOperations(o.m_stmOperations)
		,m_fieldStatistics(o.m_fieldStatistics),m_titleFields(o.m_titleFields)
		,m_stmStack(o.m_stmStack)
	{
		m_nodeScanner.init( m_nodear.data(), m_nodear.size());
	}
	ProximityWeightingContext()
		:m_config()
		,m_nofPostings(0),m_nofWeightedNeighbours(0)
		,m_hasPunctuation(false),m_docno(0),m_field()
		,m_nodear()
		,m_stmOperations()
		,m_fieldStatistics(),m_titleFields()
		,m_stmStack()
		,m_nodeScanner()
	{}

	void init(
		PostingIteratorInterface** postings, int nofPostings, const Config& config,
		PostingIteratorInterface* eos_postings, strus::Index docno, const strus::IndexRange& field);

	void initStructures( StructIteratorInterface* structIterator);
	void collectFieldStatistics();

	struct FieldStatistics
	{
		strus::IndexRange field;
		double ff[ MaxNofArguments];

		FieldStatistics( const strus::IndexRange& field_)
			:field(field_)
		{
			std::memset( &ff, 0, sizeof(ff));
		}
		FieldStatistics()
			:field()
		{
			std::memset( &ff, 0, sizeof(ff));
		}
		FieldStatistics( const FieldStatistics& o)
			:field(o.field)
		{
			std::memcpy( &ff, &o.ff, sizeof(ff));
		}
	};

	struct FeatureWeights
	{
		double ar[ MaxNofArguments];

		FeatureWeights()
		{
			std::memset( &ar, 0, sizeof(ar));
		}
		FeatureWeights( const FeatureWeights& o)
		{
			std::memcpy( &ar, &o.ar, sizeof(ar));
		}
		double operator[]( int featidx) const
		{
			return ar[ featidx];
		}
	};
	
	std::vector<FieldStatistics>::const_iterator stats_begin() const
		{return m_fieldStatistics.begin();}
	std::vector<FieldStatistics>::const_iterator stats_end() const
		{return m_fieldStatistics.end();}

	std::vector<strus::IndexRange>::const_iterator titles_begin() const
		{return m_titleFields.begin();}
	std::vector<strus::IndexRange>::const_iterator titles_end() const
		{return m_titleFields.end();}
	
	strus::IndexRange getBestPassage( const FeatureWeights& featureWeights) const;

private:
	double ff_weight( const Node& nd) const;
	void initNeighbourMatches();
	void touchTitleNode( std::vector<Node>::iterator ni, const strus::IndexRange& headerField, const strus::IndexRange& contentField);

	/// \note Operation has two instances, an open operation and a close operation
	///	Open is defined as (startpos,startpos) pair and close as (startpos,endpos) pair
	///	This definition makes the order of the operations in correct order established by a simple sort by (less(endpos),greater(startpos),less(structidx))
	struct StmOperation
	{
		Position startpos;
		Position endpos;
		unsigned short fieldidx;

		StmOperation( Position startpos_, int fieldidx_)
			:startpos(startpos_),endpos(startpos_),fieldidx(fieldidx_){}
		StmOperation( Position startpos_, Position endpos_, int fieldidx_)
			:startpos(startpos_),endpos(endpos_),fieldidx(fieldidx_){}
		StmOperation( const StmOperation& o)
			:startpos(o.startpos),endpos(o.endpos),fieldidx(o.fieldidx){}

		bool open() const
		{
			return startpos == endpos;
		}
		bool operator < (const StmOperation& o) const
		{
			return endpos == o.endpos
				? startpos == o.startpos
					? fieldidx < o.fieldidx
					: startpos > o.startpos
				: endpos < o.endpos;
		}
	};

private:
	Config m_config;
	PostingIteratorInterface* m_postings[ MaxNofArguments];
	int m_nofPostings;
	int m_nofWeightedNeighbours;
	bool m_hasPunctuation;
	strus::Index m_docno;
	strus::IndexRange m_field;
	std::vector<Node> m_nodear;
	std::vector<StmOperation> m_stmOperations;
	std::vector<FieldStatistics> m_fieldStatistics;
	std::vector<strus::IndexRange> m_titleFields;
	std::vector<int> m_stmStack;
	NodeScanner m_nodeScanner;
};

}//namespace
#endif

