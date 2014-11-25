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
#include "strus/postingIteratorInterface.hpp"
#include "postingIteratorReference.hpp"
#include "weightingFunctionReference.hpp"
#include "accumulatorArgument.hpp"
#include <vector>
#include <list>
#include <limits>
#include <boost/dynamic_bitset.hpp>

namespace strus
{
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class QueryProcessorInterface;

/// \class Accumulator
/// \brief Accumulator for weights of matches
class Accumulator
{
public:
	/// \brief Constructor
	Accumulator(
			const QueryProcessorInterface* qproc_,
			MetaDataReaderInterface* metadata_,
			std::size_t maxNofRanks_,
			std::size_t maxDocumentNumber_);

	~Accumulator()
	{}

	void addSelector(
			const PostingIteratorInterface& iterator);
	
	void addRanker(
			float factor,
			const std::string& function,
			const std::vector<float>& parameter,
			const PostingIteratorInterface& iterator);

	bool nextRank(
			Index& docno,
			unsigned int& selectorState, 
			float& weight);

private:
	bool isRelevantSelectionFeature( PostingIteratorInterface& itr) const;

private:
	const QueryProcessorInterface* m_queryprocessor;
	MetaDataReaderInterface* m_metadata;
	std::vector<PostingIteratorReference> m_selectors;
	unsigned int m_selectoridx;
	Index m_docno;
	std::vector<AccumulatorArgument> m_rankers;
	boost::dynamic_bitset<> m_visited;
	std::size_t m_maxNofRanks;
	Index m_maxDocumentNumber;
};

}//namespace
#endif

