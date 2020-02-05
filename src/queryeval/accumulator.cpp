/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "accumulator.hpp"
#include "strus/weightedDocument.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/invAclIteratorInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/scalarFunctionInstanceInterface.hpp"
#include "private/internationalization.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <iostream>

using namespace strus;

void Accumulator::addSelector(
		PostingIteratorInterface* iterator, int setindex)
{
	m_selectorPostings.push_back( SelectorPostings( false/*negative*/, setindex, iterator));
}

void Accumulator::addFeatureRestriction( PostingIteratorInterface* iterator, bool isNegative)
{
	m_featureRestrictions.push_back( SelectorPostings( isNegative, iterator));
}

void Accumulator::addWeightingElement(
		WeightingFunctionContextInterface* function_)
{
	if (m_weightingElements.size() >= Constants::MaxNofWeightingElements)
	{
		throw strus::runtime_error(_TXT("number of weighting elements exceeds maximum size: %d"), (int)Constants::MaxNofWeightingElements);
	}
	m_weightingElements.push_back( WeightingElement( function_));
}

void Accumulator::addAlternativeAclRestriction(
		const Reference<InvAclIteratorInterface>& iterator)
{
	m_aclRestrictions.push_back( iterator);
}

void Accumulator::defineWeightingVariableValue( std::size_t index, const std::string& varname, double value)
{
	m_weightingElements[ index]->setVariableValue( varname, value);
}

bool Accumulator::nextRank(
		Index& docno,
		unsigned int& selectorState)
{
	// For all selectors:
	std::vector<SelectorPostings>::const_iterator
		si = m_selectorPostings.begin() + m_selectoridx,
		se = m_selectorPostings.end();
	if (si == se)
	{
		throw std::runtime_error( _TXT( "query has no valid selection set defined"));
	}
	while (si != se)
	{
		// Select candidate document:
		if (m_evaluationSetIterator)
		{
			// ... we evaluate the query on a document subset defined by a posting iterator
			Index dn = m_docno+1;
			do
			{
				m_docno = m_evaluationSetIterator->skipDoc( dn);
				dn = si->postings->skipDoc( m_docno);
			}
			while (m_docno != 0 && dn != 0 && dn != m_docno);
		}
		else
		{
			// ... we evaluate the query on all documents
			m_docno = si->postings->skipDoc( m_docno+1);
		}
		if (!m_docno)
		{
			++si;
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
		m_visited.set( m_docno-1);

		// Check if any ACL restriction (alternatives combined with OR):
		if (m_aclRestrictions.size())
		{
			std::vector<Reference<InvAclIteratorInterface> >::iterator
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
		++m_nofDocumentsVisited;

		// Check meta data restrictions:
		if (m_metaDataRestriction.get() && !m_metaDataRestriction->match(m_docno))
		{
			continue;
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
		}
		if (ri != re) continue;

		// Init result:
		docno = m_docno;
		selectorState = m_selectorPostings[ m_selectoridx].setindex;
		++m_nofDocumentsRanked;

		std::vector<WeightingElement>::iterator
			ai = m_weightingElements.begin(), ae = m_weightingElements.end();
		const std::vector<WeightedField>* weightedFields = 0;
		int weightedFieldsIndex = -1;
		double weights[ Constants::MaxNofWeightingElements];
		// Calculate a weight for every element and call the weighting formula with the result:
		for (std::size_t aidx=0; ai != ae; ++ai,++aidx)
		{
			const std::vector<WeightedField>& wf = (*ai)->call( m_docno);
			if (wf.size() == 1 && !wf[0].field().defined())
			{
				weights[ aidx] = wf[0].weight();
			}
			else if (wf.size() == 0)
			{
				weights[ aidx] = 0.0;
			}
			else if (weightedFields != 0)
			{
				throw std::runtime_error(_TXT("more than one weighting functions defining a field with a score"));
			}
			else
			{
				weights[ aidx] = 0.0;
				weightedFields = &wf;
				weightedFieldsIndex = aidx;
			}
		}
		if (m_weightingFormula)
		{
			if (weightedFields)
			{
				std::vector<WeightedField>::const_iterator
					wi = weightedFields->begin(), we =  weightedFields->end();
				for (; wi != we; ++wi)
				{
					weights[ weightedFieldsIndex] = wi->weight();
					double fres = m_weightingFormula->call( weights, m_weightingElements.size());
					m_ranker.insert( WeightedDocument( m_docno, wi->field(), fres));
				}
			}
			else
			{
				double fres = m_weightingFormula->call( weights, m_weightingElements.size());
				m_ranker.insert( WeightedDocument( m_docno, strus::IndexRange(), fres));
			}
		}
		else
		{
			double weight = 0.0;
			int fi = 0, fe = m_weightingElements.size();
			for (; fi != fe; ++fi)
			{
				weight += weights[ fi];
			}
			if (weightedFields)
			{
				std::vector<WeightedField>::const_iterator
					wi = weightedFields->begin(), we =  weightedFields->end();
				for (; wi != we; ++wi)
				{
					m_ranker.insert( WeightedDocument( m_docno, wi->field(), weight + wi->weight()));
				}
			}
			else if (weight != 0.0)
			{
				m_ranker.insert( WeightedDocument( m_docno, strus::IndexRange(), weight));
			}
		}
		return true;
	}
	return false;
}


