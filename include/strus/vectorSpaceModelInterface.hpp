/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for vector space model operations needed for efficient similar vector retrieval in the strus storage
#ifndef _STRUS_VECTOR_SPACE_MODEL_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_INTERFACE_HPP_INCLUDED
#include <bitset>
#include <vector>

namespace strus {

/// \brief Interface for vector space model operations needed for efficient similar vector retrieval in the strus storage
class VectorSpaceModelInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInterface(){}

	/// \brief Create a new vector space model instance
	/// \return the instance (with ownership)
	virtual VectorSpaceModelInstanceInterface* createModel() const=0;
};


