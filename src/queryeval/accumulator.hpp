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
#ifndef _STRUS_QUERYEVAL_ACCUMULATOR_HPP_INCLUDED
#define _STRUS_QUERYEVAL_ACCUMULATOR_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "private/utils.hpp"
#include "metaDataRestriction.hpp"
#include <vector>
#include <list>
#include <limits>

namespace strus
{
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class InvAclIteratorInterface;

/// \class Accumulator
/// \brief Accumulator for weights of matches
class Accumulator
{
public:
	/// \brief Constructor
	Accumulator(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata_,
			const std::vector<MetaDataRestriction>& metaDataRestrictionSets_,
			std::size_t maxNofRanks_,
			std::size_t maxDocumentNumber_)
		:m_storage(storage_)
		,m_metadata(metadata_)
		,m_metaDataRestrictionSets(metaDataRestrictionSets_)
		,m_selectoridx(0)
		,m_docno(0)
		,m_visited(maxDocumentNumber_)
		,m_maxNofRanks(maxNofRanks_)
		,m_maxDocumentNumber(maxDocumentNumber_)
		,m_evaluationSetIterator(0)
	{}

	~Accumulator(){}

	void defineEvaluationSet( PostingIteratorInterface* iterator)
	{
		m_evaluationSetIterator = iterator;
	}

	void addSelector( PostingIteratorInterface* iterator, int setindex);

	void addFeature(
			float weight,
			WeightingFunctionContextInterface* function_);

	void addFeatureRestriction( PostingIteratorInterface* iterator, bool isNegative);

	void addAlternativeAclRestriction( InvAclIteratorInterface* iterator);

	bool nextRank( Index& docno, unsigned int& selectorState, float& weight);

private:
	bool isRelevantSelectionFeature( PostingIteratorInterface& itr) const;

private:
	struct WeightingFeature
	{
		Reference< WeightingFunctionContextInterface> executionContext;
		float weight;

		WeightingFeature( WeightingFunctionContextInterface* fc, float w)
			:executionContext(fc),weight(w){}
		WeightingFeature( const WeightingFeature& o)
			:executionContext(o.executionContext),weight(o.weight){}
	};

	struct SelectorPostings
	{
		bool isNegative;
		int setindex;
		PostingIteratorInterface* postings;

		SelectorPostings( bool isNegative_, int setindex_, PostingIteratorInterface* postings_)
			:isNegative(isNegative_),setindex(setindex_),postings(postings_){}
		SelectorPostings( bool isNegative_, PostingIteratorInterface* postings_)
			:isNegative(isNegative_),setindex(0),postings(postings_){}
		SelectorPostings( const SelectorPostings& o)
			:isNegative(o.isNegative),setindex(o.setindex),postings(o.postings){}
	};

	const StorageClientInterface* m_storage;
	MetaDataReaderInterface* m_metadata;
	std::vector<MetaDataRestriction> m_metaDataRestrictionSets;
	std::vector<WeightingFeature> m_weightingFeatures;
	std::vector<SelectorPostings> m_selectorPostings;
	std::vector<SelectorPostings> m_featureRestrictions;
	std::vector<InvAclIteratorInterface*> m_aclRestrictions;
	unsigned int m_selectoridx;
	Index m_docno;
	utils::DynamicBitset m_visited;
	std::size_t m_maxNofRanks;
	Index m_maxDocumentNumber;
	PostingIteratorInterface* m_evaluationSetIterator;
};

}//namespace
#endif

