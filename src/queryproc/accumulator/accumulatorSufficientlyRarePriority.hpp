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
#ifndef _STRUS_SUFFICIENTLY_RARE_PRIORITY_ACCUMULATOR_HPP_INCLUDED
#define _STRUS_SUFFICIENTLY_RARE_PRIORITY_ACCUMULATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "accumulatorReference.hpp"
#include "strus/iteratorInterface.hpp"
#include "iteratorReference.hpp"
#include <vector>

namespace strus
{

/// \class SufficientlyRarePriorityAccumulator
/// \brief Accumulator for ranks that priorities input accumulators returning matches that are occurring "sufficiently rare"
class SufficientlyRarePriorityAccumulator
	:public AccumulatorInterface
{
public:
	class PrioritisedIterator
	{
	public:
		double priority;
		IteratorReference itr;

		explicit PrioritisedIterator( IteratorReference itr_, double priority_)
			:priority(priority_)
			,itr(itr_){}

		void set( Index docno, Index follow_docno)
		{
			double ww = (follow_docno - docno);
			priority += ww * ww;
		}

		bool operator < ( const PrioritisedIterator& o) const
		{
			return (priority < o.priority);
		}
	};

	SufficientlyRarePriorityAccumulator( Index maxDocno_, const std::vector<IteratorReference>& arg_);
	virtual ~SufficientlyRarePriorityAccumulator(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weight_);

	virtual Index skipDoc( const Index& docno);
	virtual double weight();

private:
	void recalculatePriorityList();

private:
	std::vector<PrioritisedIterator> m_iterPrioList;
	std::vector<PrioritisedIterator> m_nextPrioList;
	Index m_maxDocno;
	unsigned int m_count;
	double m_weight;
};

}//namespace
#endif

