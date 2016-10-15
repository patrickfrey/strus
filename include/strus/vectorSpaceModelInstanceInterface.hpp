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
#include <vector>

namespace strus {

/// \brief Instance interface for mapping vectors of floating point numbers of a given dimension to a list of features. The mapping function is created in an unsupervised way.
class VectorSpaceModelInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInstanceInterface(){}

	/// \brief Map a vector to a set of features represented as numbers
	/// \param[in] vec vector to calculate the features from
	/// \return the resulting feature indices (indices of learnt features starting from 1)
	virtual std::vector<unsigned int> mapVectorToFeatures( const std::vector<double>& vec) const=0;

	/// \brief Map an index of the training vectors to a set of features represented as numbers
	/// \param[in] index index of vector in the order of insertion with VectorSpaceModelBuilderInterface::addVector(const std::string& name,const std::vector<double>& vec) starting from 0
	/// \return the resulting feature indices (indices of learnt features starting from 1)
	virtual std::vector<unsigned int> mapIndexToFeatures( unsigned int index) const=0;

	/// \brief Map a feature index to the list of indices of training vectors it represents
	/// \param[in] feature index (indices of learnt features starting from 1) 
	/// \return the resulting vector indices (index is order of insertion with VectorSpaceModelBuilderInterface::addVector(const std::string& name, const std::vector<double>& vec) starting from 0)
	virtual std::vector<unsigned int> mapFeatureToIndices( unsigned int feature) const=0;

	/// \brief Get the number of features learned
	/// \return the number of features and also the maximum number assigned to a feature
	virtual unsigned int nofFeatures() const=0;

	/// \brief Get the number of samples used for learning
	/// \return the number of samples
	virtual unsigned int nofSamples() const=0;

	/// \brief Get the name of a sample used for learning
	/// \param[in] index index of the sample to get the name of (index is order of insertion with VectorSpaceModelBuilderInterface::addVector(const std::string& name, const std::vector<double>& vec) starting from 0)
	/// \return the name of the sample
	virtual std::string sampleName( unsigned int index) const=0;

	/// \brief Get the configuration of this model
	/// \return the configuration string
	virtual std::string config() const=0;
};

}//namespace
#endif


