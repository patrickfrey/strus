/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Instance interface for mapping vectors of floating point numbers of a given dimension to a list of features. The mapping function is created in an unsupervised way.
#ifndef _STRUS_VECTOR_SPACE_MODEL_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>

namespace strus {

/// \brief Instance interface for mapping vectors of floating point numbers of a given dimension to a list of features. The mapping function is created in an unsupervised way.
class VectorSpaceModelInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInstanceInterface(){}

	/// \brief Map a vector to a set of features represented as numbers.
	/// \param[in] vec vector to calculate the features from
	/// \return the resulting features.
	/// \note The value of the features is depending on the model and not defined from outside. Feature learning must therefore happen in an unsupervised way.
	virtual std::vector<Index> mapVectorToFeatures( const std::vector<double>& vec) const=0;
};

}//namespace
#endif


