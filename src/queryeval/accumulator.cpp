#include "accumulator.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageInterface.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cmath>

using namespace strus;

Accumulator::Accumulator(
		const QueryProcessorInterface* qproc_,
		MetaDataReaderInterface* metadata_,
		std::size_t maxNofRanks_,
		std::size_t maxDocumentNumber_)
	:m_queryprocessor(qproc_)
	,m_metadata(metadata_)
	,m_selectoridx(0)
	,m_docno(0)
	,m_visited(maxDocumentNumber_)
	,m_maxNofRanks(maxNofRanks_)
	,m_maxDocumentNumber(maxDocumentNumber_)
{}

void Accumulator::addSelector(
		const PostingIteratorInterface& iterator)
{
	PostingIteratorReference itr( iterator.copy());
	m_selectors.push_back( itr);
}

void Accumulator::addRanker(
		float factor,
		const std::string& function,
		const std::vector<float>& parameter,
		const PostingIteratorInterface& iterator)
{
	WeightingFunctionReference weighting(
		m_queryprocessor->createWeightingFunction( function, parameter, m_metadata));
	PostingIteratorReference itr( iterator.copy());

	m_rankers.push_back( AccumulatorArgument( factor, weighting, itr));
}


bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState,
		float& weight)
{
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
		// Test if it already has been visited:
		if (m_docno > m_maxDocumentNumber || m_visited.test( m_docno-1))
		{
			// ... documents with docno bigger than m_maxDocumentNumber 
			//	were just inserted and are not respected in this query.
			continue;
		}
		m_visited[ m_docno-1] = true;

		m_metadata->skipDoc( m_docno);

		docno = m_docno;
		selectorState = m_selectoridx+1;
		weight = 0.0;

		// Add a weight for every ranker that has a match:
		std::vector<AccumulatorArgument>::iterator ai = m_rankers.begin(), ae = m_rankers.end();

		for (; ai != ae; ++ai)
		{
			if (m_docno == ai->itr->skipDoc( m_docno))
			{
				weight += ai->function->call( *ai->itr) * ai->factor;
			}
		}
		return true;
	}
	return false;
}

