#include "accumulator.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cmath>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

Accumulator::Accumulator(
		const QueryProcessorInterface* qproc_,
		std::size_t maxNofRanks_)
	:m_queryprocessor(qproc_)
	,m_selectoridx(0)
	,m_docno(0)
	,m_maxNofRanks(maxNofRanks_)
	,m_called(false)
{}

void Accumulator::addSelector(
		const IteratorInterface& iterator)
{
	IteratorReference itr( iterator.copy());
	m_selectors.push_back( itr);
}

void Accumulator::addRanker(
		float factor,
		const std::string& function,
		const std::vector<float>& parameter,
		const IteratorInterface& iterator)
{
	WeightingFunctionReference weighting(
		m_queryprocessor->createWeightingFunction( function, parameter));
	IteratorReference itr( iterator.copy());

	m_rankers.push_back( AccumulatorArgument( factor, weighting, itr));
}


bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState,
		float& weight)
{
	if (!m_called)
	{
		std::vector<IteratorReference>::const_iterator
			si = m_selectors.begin(), 
			se = m_selectors.end();
		std::size_t nofSelElems = 0;
		for (; si != se; ++si)
		{
			Index df = (*si)->documentFrequency();
			if (df > (Index)nofSelElems)
			{
				nofSelElems = (std::size_t)df;
				if (nofSelElems > m_maxNofRanks) break;
			}
		}
		m_visited.reserve( nofSelElems);
		m_called = true;
	}
	// For all selectors:
	while (m_selectoridx < m_selectors.size())
	{
		// Select candidate document:
		m_docno = m_selectors[ m_selectoridx]->skipDoc( m_docno+1);
		if (!m_docno)
		{
			++m_selectoridx;
			continue;
		}
		if (m_visited.find( m_docno) != m_visited.end())
		{
			continue;
		}

		// Mark it as visited:
		m_visited.insert( docno = m_docno);
		selectorState = m_selectoridx+1;
		weight = 0.0;

		// Add a weight for every ranker that has a match:
		std::vector<AccumulatorArgument>::iterator ai = m_rankers.begin(), ae = m_rankers.end();

		for (; ai != ae; ++ai)
		{
			if (docno == ai->itr->skipDoc( docno))
			{
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "MATCHES " << docno << ":";
				for (Index pp=ai->itr->skipPos(0); pp != 0; pp=ai->itr->skipPos(pp+1))
				{
					std::cerr << ' ' << pp;
				}
				std::cerr << std::endl;
#endif
				weight += ai->function->call( *ai->itr) * ai->factor;
			}
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "WEIGHT = " << weight << std::endl;
#endif
		}
		return true;
	}
	return false;
}

