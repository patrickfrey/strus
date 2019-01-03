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
#include "strus/vectorQueryResult.hpp"
#include <vector>
#include <limits>
#include <cmath>

namespace strus {

/// \brief Interface to repository for vectors and a feature concept relation model previously created with a builder
class VectorStorageSearchInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageSearchInterface(){}

	/// \brief Find all features that are within maximum simiarity distance of the model (or at least try to with best effort, if the model is probabilistic).
	/// \param[in] vec vector to calculate the features from
	/// \param[in] maxNofResults limits the number of results returned
	virtual std::vector<VectorQueryResult> findSimilar( const std::vector<float>& vec, unsigned int maxNofResults) const=0;

	/// \brief Find all features from a list of candidates that are within maximum simiarity distance of the model.
	/// \param[in] candidates list of candidates to search in
	/// \param[in] vec vector to calculate the features from
	/// \param[in] maxNofResults limits the number of results returned
	virtual std::vector<VectorQueryResult> findSimilarFromSelection( const std::vector<Index>& candidates, const std::vector<float>& vec, unsigned int maxNofResults) const=0;

	/// \brief Explicit close of the database access, if database was used
	/// \note this function is useful in interpreter context where a garbagge collection may delay the deletion of an object
	virtual void close()=0;
};

}//namespace
#endif

