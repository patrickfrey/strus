/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

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
#include "strus/queryEvalLib.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "iteratorReference.hpp"
#include "accumulatorReference.hpp"
#include "queryParser.hpp"
#include "dll_tags.hpp"
#include <map>
#include <set>
#include <boost/scoped_array.hpp>

using namespace strus;

namespace {
struct QueryContext
{
	QueryContext( QueryProcessorInterface* processor_, const std::string& querystr)
		:processor(processor_)
	{
		query.pushQuery( querystr);
	}

	struct IteratorSet
	{
		IteratorSet(){}
		IteratorSet( const IteratorSet& o)
			:m_ar(o.m_ar){}

		std::vector<IteratorReference> m_ar;
	};

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
	const IteratorInterface* getSetElement( unsigned int setIndex, unsigned int elemIndex)
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
		return si->second.m_ar[ elemIndex-1].get();
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
				typedef const IteratorInterface* IteratorInterfacePtr;
				IteratorInterfacePtr* paramar = new IteratorInterfacePtr[ rowsize];
				boost::scoped_array<IteratorInterfacePtr> paramlistref( paramar);
				paramar = paramlistref.get();

				for (std::size_t ci=0; ci < rowsize; ci++)
				{
					const Selector& paramdef = joinargset.ar()[ ri+ci];
					paramar[ ci] = getSetElement( paramdef.setIndex, paramdef.elemIndex);
				}
				joinresult.m_ar.push_back( processor->createIterator( ji->name(), ji->range(), rowsize, paramar));
			}
		}
	}

	/// \brief Create all accumulators defined in the query
	void expandAccumulators()
	{
		typedef QueryParser::AccumulateOperation AccumulateOperation;
		typedef QueryParser::AccumulateOperation::Argument Argument;
		typedef QueryProcessorInterface::WeightedAccumulator WeightedAccumulator;

		std::vector<AccumulateOperation>::const_iterator ai = query.accumulateOperations().begin(), ae = query.accumulateOperations().end();
		for (; ai != ae; ++ai)
		{
			if (accumap.find( ai->resultaccu()) != accumap.end())
			{
				throw std::runtime_error( "internal: duplicate accumulator definition");
			}

			// Build accumulator arguments:
			std::vector<AccumulatorReference> accuarg;
			boost::scoped_array<WeightedAccumulator> weightedAccusRef( new WeightedAccumulator[ ai->args().size()]);
			WeightedAccumulator* weightedAccus = weightedAccusRef.get();

			std::vector<Argument>::const_iterator gi = ai->args().begin(), ge = ai->args().end();
			for (std::size_t gidx=0; gi != ge; ++gi,++gidx)
			{
				if (gi->isAccumulator())
				{
					AccumulatorMap::const_iterator mi = accumap.find( gi->setIndex);
					if (mi == accumap.end()) throw std::runtime_error( "internal: could resolve accumulator");
					accuarg.push_back( AccumulatorReference( mi->second));
					weightedAccus[ gidx].weight = gi->weight;
					weightedAccus[ gidx].accu = accuarg.back().get();
				}
				else
				{
					IteratorSetMap::const_iterator ii = itersetmap.find( gi->setIndex);
					if (ii != itersetmap.end())
					{
						IteratorInterface const** paramar = new IteratorInterface const*[ ii->second.m_ar.size()];
						boost::scoped_array<const IteratorInterface*> paramlistref( paramar);
						paramar = paramlistref.get();
						std::size_t paramarsize = ii->second.m_ar.size();
		
						std::vector<IteratorReference>::const_iterator ai = ii->second.m_ar.begin(), ae = ii->second.m_ar.end();
						for (std::size_t aidx=0; ai != ae; ai++,aidx++)
						{
							paramar[aidx] = ai->get();
						}
						AccumulatorReference accu(
							processor->createOccurrenceAccumulator(
								gi->itrAccuOp, gi->factors, paramarsize, paramar));
						accuarg.push_back( accu);
						weightedAccus[ gidx].weight = gi->weight;
						weightedAccus[ gidx].accu = accuarg.back().get();
					}
				}
			}

			// Create the accumulator:
			lastaccu
				= accumap[ ai->resultaccu()]
				= processor->createAccumulator(
						ai->name(), ai->scale(),
						ai->args().size(), weightedAccus);
		}
	}

	typedef std::map< unsigned int, IteratorSet> IteratorSetMap;
	typedef std::map< unsigned int, AccumulatorReference> AccumulatorMap;

	QueryProcessorInterface* processor;
	QueryParser query;
	IteratorSetMap itersetmap;
	AccumulatorMap accumap;
	AccumulatorReference lastaccu;
};
}//namespace


DLL_PUBLIC std::vector<WeightedDocument>
	strus::evaluateQuery(
		QueryProcessorInterface& processor,
		const std::string& querystr,
		std::size_t maxNofRanks)
{
	QueryContext ctx( &processor, querystr);
	ctx.expandTerms();
	ctx.expandJoins();
	ctx.expandAccumulators();
	if (ctx.lastaccu.get())
	{
		return processor.getRankedDocumentList( *ctx.lastaccu, maxNofRanks);
	}
	else
	{
		return std::vector<WeightedDocument>();
	}
}




