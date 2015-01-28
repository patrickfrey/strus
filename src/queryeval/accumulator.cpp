#include "accumulator.hpp"
#include "mapFunctionParameters.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/weightingClosureInterface.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cmath>

using namespace strus;

void Accumulator::addSelector(
		PostingIteratorInterface* iterator, int setindex, bool isExpression)
{
	m_selectorPostings.push_back( SelectorPostings( isExpression, setindex, iterator));
}

void Accumulator::addFeatureRestriction( PostingIteratorInterface* iterator, bool isExpression)
{
	m_featureRestrictions.push_back( SelectorPostings( isExpression, iterator));
}

void Accumulator::addFeature(
		PostingIteratorInterface* iterator)
{
	m_functionClosures.push_back( 
		m_function->createClosure( 
			m_storage, iterator, m_metadata, m_parameter));
}

void Accumulator::addAclRestriction(
		InvAclIteratorInterface* iterator)
{
	m_aclRestrictions.push_back( iterator);
}

bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState,
		float& weight)
{
	// For all selectors:
	std::vector<SelectorPostings>::const_iterator
		si = m_selectorPostings.begin() + m_selectoridx,
		se = m_selectorPostings.end();
	while (si != se)
	{
		// Select candidate document:
		m_docno = si->postings->skipDoc( m_docno+1);
		if (!m_docno)
		{
			++si;
			++m_selectoridx;
			continue;
		}
		if (si->isExpression)
		{
			if (!si->postings->skipPos(0))
			{
				continue;
			}
		}

		// Test if it already has been visited:
		if (m_docno > m_maxDocumentNumber || m_visited.test( m_docno-1))
		{
			// ... documents with docno bigger than m_maxDocumentNumber 
			//	were just inserted and are not respected in this query.
			continue;
		}
		m_visited[ m_docno-1] = true;

		// Check meta data restrictions:
		m_metadata->skipDoc( m_docno);
		if (!matchesMetaDataRestriction( m_metaDataRestrictionSets, m_metadata))
		{
			continue;
		}

		// Check ACL restrictions:
		if (m_aclRestrictions.size())
		{
			std::vector<InvAclIteratorInterface*>::const_iterator
				ri = m_aclRestrictions.begin(), re = m_aclRestrictions.end();
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

		// Check feature restrictions:
		std::vector<SelectorPostings>::const_iterator
			ri = m_featureRestrictions.begin(),
			re = m_featureRestrictions.end();
		for (; ri != re; ++ri)
		{
			if (m_docno != ri->postings->skipDoc( m_docno))
			{
				break;
			}
			if (ri->isExpression)
			{
				if (!ri->postings->skipPos(0))
				{
					break;
				}
			}
		}
		if (ri != re) continue;

		// Init result:
		docno = m_docno;
		selectorState = m_selectorPostings[ m_selectoridx].setindex;
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

