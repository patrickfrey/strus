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
#include "accumulatorSumStatePrio.hpp"
#include <limits>

using namespace strus;

AccumulatorSumStatePrio::AccumulatorSumStatePrio(
		std::size_t nof_accu_,
		const QueryProcessorInterface::WeightedAccumulator* accu_)
	:m_weight(0.0)
{
	std::size_t ii=0;
	for (; ii<nof_accu_; ++ii)
	{
		m_accuorder.insert( SubAccumulatorIndex( ii)); 
		m_accu.push_back( accu_[ii]);
	}
}

bool AccumulatorSumStatePrio::nextRank( Index& docno_, int& state_, double& weight_)
{
AGAIN:
	std::set<SubAccumulatorIndex>::const_iterator ai = m_accuorder.begin(), ae = m_accuorder.end();
	if (ai == ae) return false;
	int next_state = ai->state;
	WeightedAccumulator& wa = m_accu[ ai->index];
	if (!wa.accu->nextRank( docno_, next_state, weight_))
	{
		if (next_state == std::numeric_limits<int>::max())
		{
			return false;
		}
		SubAccumulatorIndex firstaccu( ai->index);
		firstaccu.state = std::numeric_limits<int>::max();
		m_accuorder.erase( ai);
		m_accuorder.insert( firstaccu);
		goto AGAIN;
	}
	bool orderChanged = (ai->state != next_state);
	m_weight = weight_ * wa.weight;
	std::size_t index = ai->index;
	bool allMatch = true;

	for (++ai; ai != ae; ++ai)
	{
		wa = m_accu[ ai->index];
		if (wa.accu->skipDoc( docno_) == docno_)
		{
			m_weight += wa.weight * wa.accu->weight();
		}
		else if (state_ == 0)
		{
			// .. in the initial state we skip incomplete matches
			docno_ += 1;
			goto AGAIN;
		}
		else
		{
			allMatch = false;
		}
	}
	if (allMatch && state_ != 0)
	{
		// ... when not in the initial state we skip complete matches (because they were already returned)
		docno_ += 1;
		goto AGAIN;
	}
	if (orderChanged)
	{
		SubAccumulatorIndex firstaccu( index, next_state);
		m_accuorder.erase( m_accuorder.begin());
		m_accuorder.insert( firstaccu);
		state_ = m_accuorder[0].state + 1;
	}
	weight_ = m_weight;
	return true;
}

Index AccumulatorSumStatePrio::skipDoc( const Index& docno)
{
	std::set<SubAccumulatorIndex>::const_iterator ai = m_accuorder.begin(), ae = m_accuorder.end();
	if (ai == ae)
	{
		return 0;
	}
	WeightedAccumulator& wa = m_accu[ ai->index];
	Index min_docno = wa.accu->skipDoc( docno);
	m_weight = wa.accu->weight() * wa.weight;

	for (++ai; ai != ae; ++ai)
	{
		wa = m_accu[ ai->index];
		Index next_docno = wa.accu->skipDoc( docno);
		if (next_docno <= min_docno && next_docno != 0)
		{
			if (next_docno == min_docno)
			{
				m_weight += wa.accu->weight() * wa.weight;
			}
			else
			{
				min_docno = next_docno;
				m_weight = wa.accu->weight() * wa.weight;
			}
		}
	}
	return min_docno;
}

double AccumulatorSumStatePrio::weight()
{
	return m_weight;
}

