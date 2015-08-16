#include "accumulator.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "private/internationalization.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cmath>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

void Accumulator::addSelector(
		PostingIteratorInterface* iterator, int setindex, bool isExpression)
{
	m_selectorPostings.push_back( SelectorPostings( isExpression, false/*negative*/, setindex, iterator));
}

void Accumulator::addFeatureRestriction( PostingIteratorInterface* iterator, bool isExpression, bool isNegative)
{
	m_featureRestrictions.push_back( SelectorPostings( isExpression, isNegative, iterator));
}

void Accumulator::addFeature(
		float weight,
		WeightingFunctionContextInterface* function_)
{
	m_weightingFeatures.push_back( WeightingFeature( function_, weight));
}

void Accumulator::addAlternativeAclRestriction(
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
	if (si == se)
	{
		throw strus::runtime_error( _TXT( "query has no valid selection set defined"));
	}
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
		m_visited.set( m_docno-1);

		// Check meta data restrictions:
		m_metadata->skipDoc( m_docno);
		if (!matchesMetaDataRestriction( m_metaDataRestrictionSets, m_metadata))
		{
			continue;
		}

		// Check if any ACL restriction (alternatives combined with OR):
		if (m_aclRestrictions.size())
		{
			std::vector<InvAclIteratorInterface*>::const_iterator
				ri = m_aclRestrictions.begin(), re = m_aclRestrictions.end();
			Index nextAclMatch = 0;
			for (; ri != re; ++ri)
			{
				Index dn = (*ri)->skipDoc( m_docno);
				if (dn == m_docno)
				{
					break;
				}
				else if (dn != 0)
				{
					if (!nextAclMatch || dn < nextAclMatch)
					{
						nextAclMatch = dn;
					}
				}
			}
			if (ri == re)
			{
				if (nextAclMatch)
				{
					m_docno = nextAclMatch -1;
					continue;
				}
				else
				{
					m_docno = 0;
					++m_selectoridx;
					++si;
					continue;
				}
			}
		}

		// Check feature restrictions:
		std::vector<SelectorPostings>::const_iterator
			ri = m_featureRestrictions.begin(),
			re = m_featureRestrictions.end();
		for (; ri != re; ++ri)
		{
			if (ri->isNegative ^ (m_docno != ri->postings->skipDoc( m_docno)))
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

#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "Checkin document " << m_docno << std::endl;
#endif
		// Add a weight for every accumulator summand that has a match:
		std::vector<WeightingFeature>::iterator
			ai = m_weightingFeatures.begin(), ae = m_weightingFeatures.end();
		for (; ai != ae; ++ai)
		{
			float weight_result = ai->executionContext->call( m_docno) * ai->weight;
			weight += weight_result * ai->weight;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "weight +" << (weight_result * ai->weight) << " (" << weight_result << "*" << ai->weight << ") = " << weight << std::endl;
#endif
		}
		return true;
	}
	return false;
}

