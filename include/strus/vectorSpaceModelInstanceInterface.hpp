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

/// \brief Instance to work with vectors and a feature concept relation model previously created with a builder
class VectorSpaceModelInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInstanceInterface(){}

	/// \brief Trigger preloading of all internal tables needed (prevent delay of loading on demand at the first query)
	virtual void preload()=0;

	/// \brief Get the list of concept class names
	/// \return the list
	virtual std::vector<std::string> conceptClassNames() const=0;

	/// \brief Get the list of indices of features represented by a learnt concept feature specified as argument
	/// \param[in] conceptClass name identifying a class of contepts learnt. Used to distinguish different classes of learnt concepts. Defined by configuration of the vector space model and instantiated by the builder.
	/// \param[in] conceptid index (indices of learnt concepts starting from 1) 
	/// \return the resulting vector indices (index is order of insertion with VectorSpaceModelBuilderInterface::addFeature(const std::string& name, const std::vector<double>& vec) starting from 0)
	virtual std::vector<Index> conceptFeatures( const std::string& conceptClass, const Index& conceptid) const=0;

	/// \brief Get the number of concept features learned
	/// \param[in] conceptClass name identifying a class of contepts learnt. Used to distinguish different classes of learnt concepts. Defined by configuration of the vector space model and instantiated by the builder.
	/// \return the number of concept features and also the maximum number assigned to a feature (starting with 1)
	virtual unsigned int nofConcepts( const std::string& conceptClass) const=0;

	/// \brief Find all features that are within maximum simiarity distance of the model
	/// \param[in] vec vector to calculate the features from
	/// \param[in] maxNofResults limits the number of results returned
	virtual std::vector<Index> findSimilarFeatures( const std::vector<double>& vec, unsigned int maxNofResults) const=0;

	/// \brief Get the set of learnt concepts for a feature added with the builder
	/// \param[in] conceptClass name identifying a class of contepts learnt. Used to distinguish different classes of learnt concepts. Defined by configuration of the vector space model and instantiated by the builder.
	/// \param[in] index index of vector in the order of insertion with VectorSpaceModelBuilderInterface::addFeature(const std::string& name,const std::vector<double>& vec) starting from 0
	/// \return the resulting concept feature indices (indices of learnt concepts starting from 1)
	virtual std::vector<Index> featureConcepts( const std::string& conceptClass, const Index& index) const=0;

	/// \brief Get the vector of a feature added with the builder
	/// \param[in] index index of the feature in the order of insertion with VectorSpaceModelBuilderInterface::addFeature(const std::string& name,const std::vector<double>& vec) starting from 0
	/// \return the vector assinged to this feature index with VectorSpaceModelBuilderInterface::addFeature(const std::string& name,const std::vector<double>&)
	virtual std::vector<double> featureVector( const Index& index) const=0;

	/// \brief Get the name of a feature by its index starting from 0
	/// \param[in] index index of the feature to get the name of (index is order of insertion with VectorSpaceModelBuilderInterface::addFeature(const std::string& name, const std::vector<double>& vec) starting from 0)
	/// \return the name of the feature defined with VectorSpaceModelBuilderInterface::addFeature(const std::string& name,const std::vector<double>&)
	virtual std::string featureName( const Index& index) const=0;

	/// \brief Get the index starting from 0 of a feature by its name
	/// \param[in] name name of the feature defined with VectorSpaceModelBuilderInterface::addFeature(const std::string& name,const std::vector<double>&)
	/// \return index -1, if not found, else index of the feature to get the name of (index is order of insertion with VectorSpaceModelBuilderInterface::addFeature(const std::string& name, const std::vector<double>& vec) starting from 0)
	virtual Index featureIndex( const std::string& name) const=0;

	/// \brief Get some internal model specific attributes for a feature
	/// \param[in] name name of the attribute hardcoded by the model implementation
	/// \param[in] index index of the feature
	/// \note Useful for introspection
	virtual std::vector<std::string> featureAttributes( const std::string& name, const Index& index) const=0;

	/// \brief Get the list of internal model specific attributes that can be used for introspection
	/// \return list of names that can be used as argument for  with 'featureAttributes(const std::string&, const Index&) const'
	virtual std::vector<std::string> featureAttributeNames() const=0;

	/// \brief Get the number of feature vectors added with the builder
	/// \return the number of features
	virtual unsigned int nofFeatures() const=0;

	/// \brief Get the configuration of this model
	/// \return the configuration string
	virtual std::string config() const=0;
};

}//namespace
#endif


