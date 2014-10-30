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
#ifndef _STRUS_ACCUMULATOR_IDF_PRIORITY_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_IDF_PRIORITY_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/accumulatorInterface.hpp"
#include "strus/iteratorInterface.hpp"
#include "iteratorReference.hpp"
#include "weightingFunctionReference.hpp"
#include "accumulatorArgument.hpp"
#include "weighting/estimatedNumberOfMatchesMap.hpp"
#include <vector>
#include <list>
#include <set>
#include <limits>

namespace strus
{

/// \class AccumulatorIdfPriority
/// \brief Accumulator for ranking that prioritises input accumulators returning matches with a higher weight
class AccumulatorIdfPriority
	:public AccumulatorInterface
{
public:
	/// \brief Constructor
	explicit AccumulatorIdfPriority( const StorageInterface* storage_);

	/// \brief Copy constructor
	AccumulatorIdfPriority( const AccumulatorIdfPriority& o);

	virtual ~AccumulatorIdfPriority(){}

	virtual void addSelector(
			const IteratorInterface& iterator);
	
	virtual void addRanker(
			double factor,
			const std::string& function,
			const std::vector<float>& parameter,
			const IteratorInterface& iterator);

	virtual bool nextRank(
			Index& docno,
			unsigned int& selectorState, 
			double& weight);

private:
	const StorageInterface* m_storage;
	EstimatedNumberOfMatchesMapR m_estimatedNumberOfMatchesMap;
	std::vector<IteratorReference> m_selectors;
	unsigned int m_selectoridx;
	Index m_docno;
	std::vector<AccumulatorArgument> m_rankers;
	std::set<Index> m_visited;
};

}//namespace
#endif

