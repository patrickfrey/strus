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
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <cstring>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class StructureIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class ForwardIndexCollector;

class ProximityWeightingContext
{
public:
	enum {
		MaxNofArguments=64,
		MaxSummaryFieldSize=512
	};
	struct Config
	{
		int distance_imm;			///< ordinal position distance considered as immediate in the same sentence
		int distance_close;			///< ordinal position distance considered as close in the same sentence
		int distance_near;			///< ordinal position distance considered as near for features not in the same sentence
		float minClusterSize;			///< part [0.0,1.0] of query features considered as relevant in a group
		int nofHotspots;			///< only a part of the matches are used to calculate the structures weighted in a document. The number of hotspots defines the number of features with most neighbour features to be used to determine the document parts to weight.
		int nofSummarySentences;		///< number of sentences in a summary
		int maxNofSummarySentenceWords;		///< maximum number of words in a summary sentence
		double minFfWeight;			///< minimum ff assigned [0.0,1.0] to any feature weighted, some features are lost if you use a minimum cluster size > 0.0

		Config()
			:distance_imm(2)
			,distance_close(8)
			,distance_near(40)
			,minClusterSize(0.7)
			,nofHotspots(10)
			,nofSummarySentences(2)
			,maxNofSummarySentenceWords(20)
			,minFfWeight(0.1){}
		Config( const Config& o)
			:distance_imm(o.distance_imm)
			,distance_close(o.distance_close)
			,distance_near(o.distance_near)
			,minClusterSize(o.minClusterSize)
			,nofHotspots(o.nofHotspots)
			,nofSummarySentences(o.nofSummarySentences)
			,maxNofSummarySentenceWords(o.maxNofSummarySentenceWords)
			,minFfWeight(o.minFfWeight){}

		void setDistanceImm( int distance_imm_);
		void setDistanceClose( int distance_close_);
		void setDistanceNear( int distance_near_);
		void setMinClusterSize( float minClusterSize_);
		void setNofHotspots( float nofHotspots_);
		void setNofSummarySentences( float nofSummarySentences_);
		void setMaxNofSummarySentenceWords( float maxNofSummarySentenceWords_);
		void setMinFfWeight( double minFfWeight_);
	};

	struct Node
	{
		strus::bitset<MaxNofArguments> touched;
		strus::Index pos;
		unsigned char closeMatches;
		unsigned char immediateMatches;
		unsigned char sentenceMatches;
		unsigned char nearMatches;
		unsigned char titleScopeMatches;
		unsigned char featidx;

		Node()
			:touched()
			,pos(0)
			,closeMatches(0)
			,immediateMatches(0)
			,sentenceMatches(0)
			,nearMatches(0)
			,titleScopeMatches(0)
			,featidx(0)
		{}
		Node( int featidx_, strus::Index documentPos)
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
		Node& operator=( const Node& o)
			{touched=o.touched;pos=o.pos;closeMatches=o.closeMatches;
			immediateMatches=o.immediateMatches;sentenceMatches=o.sentenceMatches;
			nearMatches=o.nearMatches;titleScopeMatches=o.titleScopeMatches;
			featidx=o.featidx; return *this;}

		enum TouchType {ImmediateTouch,CloseTouch,NearTouch,SentenceTouch};
		inline void incrementTouchCount( TouchType tp)
		{
			switch (tp)
			{
				case ImmediateTouch: ++immediateMatches; break;
				case CloseTouch: ++closeMatches; break;
				case NearTouch: ++nearMatches; break;
				case SentenceTouch: ++sentenceMatches; break;
			}
		}
		inline int touchCount() const
		{
			return closeMatches + immediateMatches + sentenceMatches + nearMatches + titleScopeMatches;
		}
		/// \brief Comparator functor to find nodes with a position greator or equal to needle
		struct SearchCompare
		{
			SearchCompare(){}
			bool operator()( const Node& aa, strus::Index bb) const
			{
				return aa.pos < bb;
			}
		};
	};

	typedef SkipScanArray<Node,strus::Index,Node::SearchCompare> NodeScanner;

public:
	ProximityWeightingContext( const ProximityWeightingContext& o);
	ProximityWeightingContext( const Config& config_);

	const Config& config() const
	{
		return m_config;
	}

	void init(
		PostingIteratorInterface** postings, int nofPostings, PostingIteratorInterface* eos_postings, 
		strus::Index docno, const strus::IndexRange& field);

	void initStructures( StructureIteratorInterface* structIterator, strus::Index structno);
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
		double& operator[]( int featidx)
		{
			return ar[ featidx];
		}
	};
	
	std::vector<FieldStatistics>::const_iterator stats_begin() const
		{return m_fieldStatistics.begin();}
	std::vector<FieldStatistics>::const_iterator stats_end() const
		{return m_fieldStatistics.end();}

	strus::IndexRange getBestPassage( const FeatureWeights& featureWeights) const;

	struct WeightedNeighbour
	{
		strus::Index pos;
		double weight;

		WeightedNeighbour()
			:pos(0),weight(0.0){}
		WeightedNeighbour( strus::Index pos_, double weight_)
			:pos(pos_),weight(weight_){}
		WeightedNeighbour( const WeightedNeighbour& o)
			:pos(o.pos),weight(o.weight){}
	};

	std::vector<WeightedNeighbour>
		getWeightedNeighbours(
			const FeatureWeights& featureWeights,
			strus::Index dist) const;

	static std::vector<WeightedNeighbour>
		getWeightedNeighboursForSingleFeature( 
			strus::Index dist,
			PostingIteratorInterface* postings,
			PostingIteratorInterface* eos_postings, 
			strus::Index docno,
			const strus::IndexRange& field);

private:
	double ff_weight( const Node& nd) const;
	void initNeighbourMatches();
	void touchTitleNode( std::vector<Node>::iterator ni, const strus::IndexRange& headerField, const strus::IndexRange& contentField);
	void validateTouchCount();
	void markTouches( Node::TouchType touchType, std::vector<Node>::iterator ni, std::vector<Node>::iterator ne, strus::Index dist);
	void markTouchesInSentence( Node::TouchType touchType, std::vector<Node>::iterator ni, std::vector<Node>::iterator ne, strus::Index dist);

	/// \note Operation has two instances, an open operation and a close operation
	///	Open is defined as (startpos,startpos) pair and close as (startpos,endpos) pair
	///	This definition makes the order of the operations in correct order established by a simple sort by (less(endpos),greater(startpos),less(structidx))
	struct StmOperation
	{
		strus::Index startpos;
		strus::Index endpos;
		unsigned short fieldidx;

		StmOperation( strus::Index startpos_, int fieldidx_)
			:startpos(startpos_),endpos(startpos_),fieldidx(fieldidx_){}
		StmOperation( strus::Index startpos_, strus::Index endpos_, int fieldidx_)
			:startpos(startpos_),endpos(endpos_),fieldidx(fieldidx_){}
		StmOperation( const StmOperation& o)
			:startpos(o.startpos),endpos(o.endpos),fieldidx(o.fieldidx){}

		bool operator < (const StmOperation& o) const
		{
			if (endpos == o.endpos)
			{
				switch (2 * isStartTag() + o.isStartTag())
				{
					case 3://... both start tags
						return fieldidx < o.fieldidx;
					case 2://... this is start tag and the other not
						return false;
					case 1://... this is an end tag and the other is a start tag
						return true;
					case 0://... both are end tags
						if (startpos == o.startpos)
						{
							return fieldidx < o.fieldidx;
						}
						else
						{
							return startpos > o.startpos;
						}
				}
				return false;
			}
			else
			{
				return endpos < o.endpos;
			}
		}
		int isStartTag() const
		{
			return startpos == endpos;
		}
	};

private:
	Config m_config;
	PostingIteratorInterface* m_postings[ MaxNofArguments];
	int m_nofPostings;
	int m_nofWeightedNeighbours;
	int m_minClusterSize;
	unsigned char m_featureids[ MaxNofArguments];
	strus::Index m_length_postings[ MaxNofArguments];
	bool m_hasPunctuation;
	strus::Index m_docno;
	strus::IndexRange m_field;
	std::vector<Node> m_nodear;
	std::vector<StmOperation> m_stmOperations;
	std::vector<FieldStatistics> m_fieldStatistics;
	std::vector<int> m_stmStack;
	NodeScanner m_nodeScanner;
};

}//namespace
#endif

