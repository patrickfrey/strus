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

/// \brief Forward declaration
class VectorSpaceModelBuilderInterface;
/// \brief Forward declaration
class VectorSpaceModelInstanceInterface;

/// \brief Interface for mapping vectors of floating point numbers of a given dimension to a list of features. The mapping function is created in an unsupervised way.
class VectorSpaceModelInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInterface(){}

	/// \brief Delete the repository of a model
	/// \param[in] config connfiguration string of the model
	/// \return true on success
	virtual bool destroyModel( const std::string& config) const=0;

	/// \brief Create a new vector space model instance
	/// \param[in] config connfiguration string of the model
	/// \return the instance (with ownership)
	virtual VectorSpaceModelInstanceInterface* createInstance( const std::string& config) const=0;

	/// \brief Create a new vector space model builder
	/// \param[in] config configuration string of the model
	/// \return the builder (with ownership)
	virtual VectorSpaceModelBuilderInterface* createBuilder( const std::string& config) const=0;
};

}//namespace
#endif


