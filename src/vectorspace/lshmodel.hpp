/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief LSH Similarity model structure
#ifndef _STRUS_VECTOR_SPACE_MODEL_LSHMODEL_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_LSHMODEL_HPP_INCLUDED
#include "strus/base/stdint.h"
#include "simhash.hpp"
#include <vector>
#include <string>
#include <armadillo>

namespace strus {

/// \brief Structure for calculating LSH similarity
class LshModel
{
public:
	LshModel( std::size_t dim_, std::size_t nofbits_, std::size_t variations_);

	std::string tostring() const;

	std::string serialize() const;
	static LshModel* createFromSerialization( const std::string& dump);

	SimHash simHash( const arma::vec& vec) const;

private:
	static arma::mat createModelMatrix( std::size_t dim_, std::size_t nofbits_);
	LshModel( std::size_t dim_, std::size_t nofbits_, std::size_t variations_, const arma::mat& modelMatrix_, const std::vector<arma::mat>& rotations_);

private:
	std::size_t m_dim;
	std::size_t m_nofbits;
	std::size_t m_variations;
	arma::mat m_modelMatrix;
	std::vector<arma::mat> m_rotations;
};

}//namespace
#endif


