#include "accumulator.hpp"
#include "mapFunctionParameters.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/docnoIteratorInterface.hpp"
#include "strus/weightingClosureInterface.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cmath>

using namespace strus;

void Accumulator::addSelector(
		PostingIteratorInterface* iterator)
{
	m_selectorPostings.push_back( iterator);
}

void Accumulator::addFeature(
		PostingIteratorInterface* iterator)
{
	m_functionClosures.push_back( 
		m_function->createClosure( 
			m_storage, iterator, m_metadata, m_parameter));
}

void Accumulator::addRestrictionSet(
		DocnoIteratorInterface* iterator)
{
	m_restrictionSets.push_back( iterator);
}

bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState,
		float& weight)
{
	// For all selectors:
	while (m_selectoridx < m_selectorPostings.size())
	{
		// Select candidate document:
		m_docno = m_selectorPostings[ m_selectoridx]->skipDoc( m_docno+1);
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
		if (m_restrictionSets.size())
		{
			// Apply restrictions defined by document sets
			std::vector<DocnoIteratorInterface*>::const_iterator
				ri = m_restrictionSets.begin(), re = m_restrictionSets.end();
			for (; ri != re; ++ri)
			{
				Index dn = (*ri)->skipDoc( m_docno);
				if (dn != m_docno)
				{
					if (!dn)
					{
						m_docno = 0;
						++m_selectoridx;
						break;
					}
					else
					{
						m_docno = dn -1;
						break;
					}
				}
			}
			if (ri != re) continue;
		}
		m_metadata->skipDoc( m_docno);

		docno = m_docno;
		selectorState = m_selectoridx+1;
		weight = 0.0;

		// Add a weight for every accumulator summand that has a match:
		std::vector<Reference< WeightingClosureInterface> >::iterator
			ai = m_functionClosures.begin(), ae = m_functionClosures.end();
		for (; ai != ae; ++ai)
		{
			weight += (*ai)->call( m_docno);
		}
		return true;
	}
	return false;
}

