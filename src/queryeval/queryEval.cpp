/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "queryParser.hpp"
#include "queryEval.hpp"
#include <cstring>
#include <set>

using namespace strus;

struct QueryContext
{
	QueryContext( QueryProcessorInterface* processor_, const std::string& querystr)
		:processor(processor_)
	{
		query.pushQuery( querystr);
	}

	struct IteratorSet
	{
		IteratorSet()
			:m_refcnt(0){}
		IteratorSet( const IteratorSet& o)
			:m_ar(o.m_ar),m_refcnt(o.m_refcnt){}

		std::vector<IteratorReference> m_ar;
		unsigned int m_refcnt;
	};

	QueryProcessorInterface* processor;
	QueryParser query;
	typedef std::map< unsigned int, IteratorSet> IteratorSetMap;
	typedef std::map< unsigned int, AccumulatorReference> AccumulatorMap;
	IteratorSetMap itersetmap;
	AccumulatorMap accumap;
	AccumulatorReference lastaccu;

	/// \brief Create all basic terms defined in the query
	void expandTerms()
	{
		std::vector<QueryParser::Term>::const_iterator ti = query.terms().begin(), te = query.terms().end();
		for (; ti != te; ++ti)
		{
			IteratorReference itr( processor->createIterator( ti->type(), ti->value()));
			IteratorSet& set = itersetmap[ ti->resultset()];
			set.m_ar.push_back( itr);
		}
	}

	/// \brief Get a specific element of an iterator set
	IteratorReference getSetElement( unsigned int setIndex, unsigned int elemIndex)
	{
		IteratorSetMap::iterator si = itersetmap.find( setIndex);
		if (si == itersetmap.end())
		{
			throw std::runtime_error( "internal: set addressing error in query");
		}
		if (si->second.m_ar.size() <= (elemIndex-1))
		{
			throw std::runtime_error( "internal: set element addressing error in query");
		}
		++si->second.m_refcnt;
		IteratorReference rt = si->second.m_ar[ elemIndex-1];
		if (si->second.m_refcnt > 1)
		{
			rt.reset( rt->copy());
		}
		return rt;
	}

	/// \brief Create all joins defined in the query
	void expandJoins()
	{
		typedef QueryParser::JoinOperation JoinOperation;
		typedef QueryParser::JoinOperation::SelectorSet SelectorSet;
		typedef QueryParser::JoinOperation::Selector Selector;

		std::vector<JoinOperation>::const_iterator ji = query.joinOperations().begin(), je = query.joinOperations().end();
		for (; ji != je; ++ji)
		{
			IteratorSet& joinresult = itersetmap[ ji->resultset()];

			const SelectorSet& joinargset = *ji->selectorset().get();
			std::size_t rowsize = joinargset.rowsize();
			std::size_t ri = 0, re = joinargset.nofrows();
			for (; ri < re; ri+=rowsize)
			{
				std::vector<IteratorReference> paramlist;
				for (std::size_t ci=0; ci < rowsize; ci++)
				{
					const Selector& paramdef = joinargset.ar()[ ri+ci];
					paramlist.push_back( getSetElement( paramdef.setIndex, paramdef.elemIndex));
				}
				joinresult.m_ar.push_back( processor->createIterator( ji->name(), paramlist));
			}
		}
	}

	/// \brief Create all accumulators defined in the query
	void expandAccumulators()
	{
		typedef QueryParser::AccumulateOperation AccumulateOperation;
		typedef QueryParser::AccumulateOperation::Argument Argument;
		typedef QueryParser::SetElementList SetElementList;
		typedef QueryParser::SetElement SetElement;

		std::vector<AccumulateOperation>::const_iterator ai = query.accumulateOperations().begin(), ae = query.accumulateOperations().end();
		for (; ai != ae; ++ai)
		{
			if (accumap.find( ai->resultaccu()) != accumap.end())
			{
				throw std::runtime_error( "internal: duplicate accumulator definition");
			}

			// Build accumulator arguments:
			std::vector<AccumulatorReference> accuarg;
			std::vector<double> weightarg;
			std::vector<Argument>::const_iterator gi = ai->args().begin(), ge = ai->args().end();
			for (; gi != ge; ++gi)
			{
				if (gi->isAccumulator())
				{
					AccumulatorMap::const_iterator mi = accumap.find( gi->setIndex);
					if (mi == accumap.end()) throw std::runtime_error( "internal: could resolve accumulator");
					accuarg.push_back( AccumulatorReference( mi->second));
					weightarg.push_back( gi->weight);
				}
				else
				{
					IteratorSetMap::const_iterator ii = itersetmap.find( gi->setIndex);
					if (ii != itersetmap.end())
					{
						AccumulatorReference accu 
							= processor->createOccurrenceAccumulator(
								gi->itrAccuOp, ii->second.m_ar);
						accuarg.push_back( accu);
						weightarg.push_back( gi->weight);
					}
				}
			}

			// Create the accumulator:
			lastaccu
				= accumap[ ai->resultaccu()]
				= processor->createAccumulator(
					ai->name(), ai->scale(), weightarg, accuarg);
		}
	}

	/// \brief Get the best ranks as weighted by the last accumulator in the query defined
	std::vector<WeightedDocument> rankDocuments( std::size_t maxNofRanks)
	{
		std::vector<WeightedDocument> rt;
		typedef std::multiset<WeightedDocument,WeightedDocument::CompareSmaller> Ranker;
		Ranker ranker;
		std::size_t ranks = 0;

		if (!lastaccu.get()) return rt;
		AccumulatorInterface& accu = *lastaccu.get();

		Index docno = 0;
		int state = 0;
		double weigth = 0.0;

		while (accu.nextRank( docno, state, weigth))
		{
			ranker.insert( WeightedDocument( docno, weigth));
			if (ranks >= maxNofRanks)
			{
				ranker.erase( ranker.begin());
			}
			else
			{
				++ranks;
			}
		}
		Ranker::reverse_iterator ri=ranker.rbegin(),re=ranker.rend();
		for (; ri != re; ++ri)
		{
			rt.push_back( *ri);
		}
		return rt;
	}
};

std::vector<WeightedDocument>
		QueryEval::evaluate(
			QueryProcessorInterface& processor,
			const std::string& querystr,
			std::size_t maxNofRanks)
{
	QueryContext ctx( &processor, querystr);
	ctx.expandTerms();
	ctx.expandJoins();
	ctx.expandAccumulators();
	return ctx.rankDocuments( maxNofRanks);
}



