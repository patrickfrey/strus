/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for creating a mapping of floating point vectors of a defined dimension to a list of features in an unsupervised way.
#ifndef _STRUS_VECTOR_SPACE_MODEL_BUILDER_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_BUILDER_INTERFACE_HPP_INCLUDED
#include <vector>

namespace strus {

/// \brief Interface for creating a mapping of floating point vectors of a defined dimension to a list of features in an unsupervised way.
class VectorSpaceModelBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelBuilderInterface(){}

	/// \brief Add a sample vector to the model for training (unsupervised learning)
	/// \param[in] vec vector to add
	/// \note the model gets an example vector that can be used to build internal structures for the feature mapping in an unsupervised way
	virtual void addSampleVector( const std::string& name, const std::vector<double>& vec)=0;

	/// \brief Finalize all calculations for the model.
	virtual bool finalize()=0;

	/// \brief Store the model persistently.
	/// \return true on success, false if the transaction failed
	virtual bool store()=0;
};

}//namespace
#endif



