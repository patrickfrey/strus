/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for searching vectors. Separated from client interface because it requires linear scanning of candidates in memory because of the nature of the problem.
#ifndef _STRUS_VECTOR_STORGE_SEARCH_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_SEARCH_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/wordVector.hpp"
#include "strus/vectorQueryResult.hpp"
#include <vector>
#include <limits>
#include <cmath>

namespace strus {

/// \brief Search interface to repository for vectors
class VectorStorageSearchInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageSearchInterface(){}

	/// \brief Find all features that are within maximum simiarity distance.
	/// \param[in] vec vector to calculate the features from
	/// \param[in] maxNofResults limits the number of results returned
	/// \param[in] minSimilarity value between 0.0 and 1.0 specifying the minimum similarity a result should have
	/// \param[in] realVecWeights true if to calculate the real vector weights and diffs for the best matches and not an approximate value derived from the LSH bits differing
	virtual std::vector<VectorQueryResult> findSimilar( const WordVector& vec, int maxNofResults, double minSimilarity, bool realVecWeights) const=0;

	/// \brief Explicit close of the database access, if database was used
	/// \note this function is useful in interpreter context where a garbagge collection may delay the deletion of an object
	virtual void close()=0;
};

}//namespace
#endif

