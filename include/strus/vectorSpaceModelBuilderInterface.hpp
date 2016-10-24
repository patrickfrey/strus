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

/// \brief Interface for building a repository of vectors of floating point numbers representing document features and for learning of a model that relates added features to concept features.
class VectorSpaceModelBuilderInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelBuilderInterface(){}

	/// \brief Add a feature to the model for later retrieval and for learning of the concepts associated to features
	/// \param[in] vec vector to add
	virtual void addFeature( const std::string& name, const std::vector<double>& vec)=0;

	/// \brief Do a commit and ensure the persistent storage the model data created till now
	/// \return true on success, false if the commit failed
	virtual bool commit()=0;

	/// \brief Do the unsupervised learning of the feature concept relations and store the results persistently for later use.
	/// \return true on success, false if the operation failed
	/// \note This method includes a commit on the database, you do not have to call commit anymore.
	virtual bool finalize()=0;
};

}//namespace
#endif



