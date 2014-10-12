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
#ifndef _STRUS_ACCUMULATOR_WEIGHTED_SUM_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_WEIGHTED_SUM_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include <set>

namespace strus
{

/// \class AccumulatorWeightedSum
/// \brief Accumulator of sums of weighted results
class AccumulatorWeightedSum
	:public AccumulatorInterface
{
public:
	typedef QueryProcessorInterface::WeightedAccumulator WeightedAccumulator;

	struct SubAccumulatorIndex
	{
		explicit SubAccumulatorIndex( std::size_t index_, int state_=0)
			:state(state_)
			,index(index_){}
		SubAccumulatorIndex( const SubAccumulatorIndex& o)
			:state(o.state)
			,index(o.index){}

		bool operator < ( const SubAccumulatorIndex& o) const
		{
			if (state < o.state) return true;
			if (index < o.index) return true;
			return false;
		}

		int state;
		std::size_t index;
	};

	AccumulatorWeightedSum(
		std::size_t nof_accu_,
		const WeightedAccumulator* accu_);
	virtual ~AccumulatorWeightedSum(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weight_);

	virtual Index skipDoc( const Index& docno);
	virtual double weight();

private:
	std::set<SubAccumulatorIndex> m_accuorder;
	std::vector<WeightedAccumulator> m_accu;
	double m_weight;
};

}//namespace
#endif

