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
#ifndef _STRUS_WEIGHTED_SUM_ACCUMULATOR_HPP_INCLUDED
#define _STRUS_WEIGHTED_SUM_ACCUMULATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include <set>

namespace strus
{

/// \class SufficientlyRarePriorityAccumulator
/// \brief Accumulator for ranks that priorities input accumulators returning matches that are occurring "sufficiently rare"
class WeightedSumAccumulator
	:public AccumulatorInterface
{
public:
	struct SubAccumulator
		:public QueryProcessorInterface::WeightedAccumulator
	{
		SubAccumulator( const QueryProcessorInterface::WeightedAccumulator& o,
				std::size_t order_)
			:QueryProcessorInterface::WeightedAccumulator(o)
			,state(0){}
		SubAccumulator( const SubAccumulator& o)
			:QueryProcessorInterface::WeightedAccumulator(o)
			,state(o.state){}

		bool operator < ( const SubAccumulator& o) const
		{
			if (state < o.state) return true;
			if (order < o.order) return true;
			return false;
		}

		int state;
		std::size_t order;
	};

	WeightedSumAccumulator(
		std::size_t nof_accu_,
		const QueryProcessorInterface::WeightedAccumulator* accu_);
	virtual ~WeightedSumAccumulator(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weigth_);

	virtual Index skipDoc( const Index& docno);
	virtual double weight();

private:
	std::set<SubAccumulator> m_accu;
	double m_weight;
};

}//namespace
#endif

