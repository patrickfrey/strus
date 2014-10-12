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
#ifndef _STRUS_ACCUMULATOR_PRIORITISED_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_PRIORITISED_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "accumulatorReference.hpp"
#include "strus/iteratorInterface.hpp"
#include "iteratorReference.hpp"
#include <vector>
#include <list>
#include <set>

namespace strus
{

/// \class AccumulatorPrioritised
/// \brief Accumulator for ranks that prioritises input accumulators returning matches with a higher weight
class AccumulatorPrioritised
	:public AccumulatorInterface
{
public:
	/// \class IteratorPrioritised
	/// \brief Iterator with priority attached
	class IteratorPrioritised
	{
	public:
		explicit IteratorPrioritised( IteratorReference itr_, double weight_=0.0, bool finished_=false)
			:finished(finished_)
			,weight(weight_)
			,itr(itr_){}

		bool operator < ( const IteratorPrioritised& o) const
		{
			return finished?false:(weight < o.weight);
		}

		bool finished;
		double weight;
		IteratorReference itr;
	};

	/// \brief Constructor
	/// \param[in] maxDocno_ maximum document number in collection for selecting random matches for the initial weight calculation
	/// \param[in] arg_ argument iterators to accumulate matches from
	AccumulatorPrioritised( 
		Index maxDocno_,
		const std::vector<IteratorReference>& arg_);
	virtual ~AccumulatorPrioritised(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weight_);

	virtual Index skipDoc( const Index& docno);
	virtual double weight();


private:
	enum {RecalculatePrioListLoopSize=50};
	double calcInitialWeight( const IteratorReference& itr);
	void recalculatePriorityList();

private:
	std::list<IteratorPrioritised> m_iterPrioList;
	std::set<Index> m_visited;
	Index m_maxDocno;
	unsigned int m_loopCount;
	double m_weight;
};

}//namespace
#endif

