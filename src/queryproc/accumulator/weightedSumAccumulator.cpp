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
#include "weightedSumAccumulator.hpp"
#include <limits>

using namespace strus;

WeightedSumAccumulator::WeightedSumAccumulator(
		std::size_t nof_accu_,
		const QueryProcessorInterface::WeightedAccumulator* accu_)
	:m_weight(0.0)
{
	std::size_t ii=0;
	for (; ii<nof_accu_; ++ii)
	{
		m_accu.insert( SubAccumulator( accu_[ii], ii));
	}
}

bool WeightedSumAccumulator::nextRank( Index& docno_, int& state_, double& weigth_)
{
AGAIN:
	std::set<SubAccumulator>::iterator ai = m_accu.begin(), ae = m_accu.end();
	if (ai == ae) return false;
	int prev_state = ai->state;
	if (!ai->accu->nextRank( docno_, ai->state, weigth_))
	{
		if (ai->state == std::numeric_limits<int>::max())
		{
			return false;
		}
		SubAccumulator firstaccu( *ai);
		firstaccu.state = std::numeric_limits<int>::max();
		m_accu.erase( ai);
		m_accu.insert( firstaccu);
		goto AGAIN;
	}
	bool orderChanged = (ai->state != prev_state);
	weight_ *= ai->weight;

	for (++ai; ai != ae; ++ai)
	{
		if (ai->accu->skipDoc( docno_) == docno_)
		{
			weight_ += ai->weight * ai->accu->weight();
		}
	}
	if (orderChanged)
	{
		SubAccumulator firstaccu( m_accu[0]);
		m_accu.erase( m_accu->begin());
		m_accu.insert( firstaccu);
	}
	m_weight = weight_;
	return true;
}

Index WeightedSumAccumulator::skipDoc( const Index& docno)
{
	std::set<SubAccumulator>::iterator ai = m_accu.begin(), ae = m_accu.end();
	if (ai == ae)
	{
		return 0;
	}
	Index min_docno = ai->accu->skipDoc( docno);
	m_weight = 0.0;

	for (++ai; ai != ae; ++ai)
	{
		Index next_docno = ai->accu->skipDoc( docno);
		if (next_docno <= min_docno && next_docno != 0)
		{
			if (next_docno == min_docno)
			{
				m_weight += ai->accu->weight() * ai->weight;
			}
			else
			{
				min_docno = next_docno;
				m_weight = ai->accu->weight() * ai->weight;
			}
		}
	}
	return min_docno;
}

double WeightedSumAccumulator::weight()
{
	return m_weight;
}

