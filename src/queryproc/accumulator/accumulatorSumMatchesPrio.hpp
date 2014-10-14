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
#ifndef _STRUS_ACCUMULATOR_SUM_MATCHES_PRIO_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_SUM_MATCHES_PRIO_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "accumulatorReference.hpp"
#include "strus/iteratorInterface.hpp"
#include "accumulatorReference.hpp"
#include <vector>
#include <list>
#include <set>

namespace strus
{

/// \class AccumulatorSumMatchesPrio
/// \brief Accumulator for ranking that prioritises input accumulators returning matches with a higher weight
class AccumulatorSumMatchesPrio
	:public AccumulatorInterface
{
public:
	/// \class AccumulatorWithProbabilityWeight
	/// \brief Accumulator with a probability estimate for using it in a priority queue
	class AccumulatorWithWeight
	{
	public:
		explicit AccumulatorWithWeight( AccumulatorReference itr_, double weight_=0.0, bool finished_=false)
			:finished(finished_)
			,weight(weight_)
			,itr(itr_){}

		bool operator < ( const AccumulatorWithWeight& o) const
		{
			return finished?false:(weight < o.weight);
		}

		double weight;
		AccumulatorReference itr;
	};

	/// \brief Constructor
	/// \param[in] arg_ argument iterators to accumulate matches from
	AccumulatorSumMatchesPrio( 
		std::size_t nof_accu_,
		const WeightedAccumulator* accu_);

	/// \brief Copy constructor
	AccumulatorSumMatchesPrio( const AccumulatorSumMatchesPrio& o);

	virtual ~AccumulatorSumMatchesPrio(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weight_);

	virtual Index skipDoc( const Index& docno);
	virtual double weight();

	virtual AccumulatorInterface* copy() const
	{
		return new AccumulatorSumMatchesPrio( *this);
	}

private:
	double calcInitialWeight( const IteratorReference& itr);
	void recalculatePriorityList();

private:
	std::list<AccumulatorWithProbabilityWeight> m_iterPrioList;
	std::set<Index> m_visited;
	Index m_maxDocno;
	unsigned int m_loopCount;
	double m_weight;
};

}//namespace
#endif

