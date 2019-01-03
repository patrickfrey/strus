/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Client interface for mapping vectors of floating point numbers of a given dimension to a list of features.
#ifndef _STRUS_VECTOR_STORGE_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_CLIENT_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class VectorStorageSearchInterface;
/// \brief Forward declaration
class VectorStorageTransactionInterface;

/// \brief Interface to a repository for vectors and a feature concept relation model previously created with a builder
class VectorStorageClientInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageClientInterface(){}

	/// \brief Create a search interface for scanning the vectors for similarity to a vector
	/// \param[in] range_from start range of the features for the searcher (possibility to split into multiple instances)
	/// \param[in] range_to end of range of the features for the searcher (possibility to split into multiple instances)
	/// \return the search interface (with ownership)
	virtual VectorStorageSearchInterface* createSearcher( const Index& range_from, const Index& range_to) const=0;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface (with ownership)
	/// \note this function is thread safe, multiple concurrent transactions are allowed 
	virtual VectorStorageTransactionInterface* createTransaction()=0;

	/// \brief Get the list of concept class names defined
	/// \return the list
	virtual std::vector<std::string> conceptClassNames() const=0;

	/// \brief Get the list of indices of features represented by a learnt concept feature specified as argument
	/// \param[in] conceptClass name identifying a class of concepts learnt. Concept classes are defined by configuration of the vector storage and instantiated by the storage builder.
	/// \param[in] conceptid index (indices of learnt concepts starting from 1) 
	/// \return the resulting vector indices (index is order of insertion with VectorStorageBuilderInterface::addFeature(const std::string& name, const std::vector<float>& vec) starting from 0)
	virtual std::vector<Index> conceptFeatures( const std::string& conceptClass, const Index& conceptid) const=0;

	/// \brief Get the number of concept features learned for a class
	/// \param[in] conceptClass name identifying a class of concepts learnt. Concept classes are defined by configuration of the vector storage and instantiated by the builder.
	/// \return the number of concept features and also the maximum number assigned to a feature (starting with 1)
	virtual unsigned int nofConcepts( const std::string& conceptClass) const=0;

	/// \brief Get the set of learnt concepts for a feature defined
	/// \param[in] conceptClass name identifying a class of concepts learnt. Used to distinguish different classes of learnt concepts. Defined by configuration of the vector storage and instantiated by the storage builder.
	/// \param[in] index index of vector (featureIndex( const std::string&) const)
	/// \return the resulting concept feature indices (indices of learnt concepts starting from 1)
	virtual std::vector<Index> featureConcepts( const std::string& conceptClass, const Index& index) const=0;

	/// \brief Get the vector assigned to a feature 
	/// \param[in] index index of the feature in the order of insertion with VectorStorageBuilderInterface::addFeature(const std::string& name,const std::vector<float>& vec) starting from 0
	/// \return the vector assinged to this feature index with VectorStorageBuilderInterface::addFeature(const std::string& name,const std::vector<float>&)
	virtual std::vector<float> featureVector( const Index& index) const=0;

	/// \brief Get the name of a feature by its index starting from 0
	/// \param[in] index index of the feature to get the name of (index is order of insertion with VectorStorageBuilderInterface::addFeature(const std::string& name, const std::vector<float>& vec) starting from 0)
	/// \return the name of the feature defined with VectorStorageBuilderInterface::addFeature(const std::string& name,const std::vector<float>&)
	virtual std::string featureName( const Index& index) const=0;

	/// \brief Get the index starting from 0 of a feature by its name
	/// \param[in] name name of the feature defined with VectorStorageBuilderInterface::addFeature(const std::string& name,const std::vector<float>&)
	/// \return index -1, if not found, else index of the feature to get the name of (index is order of insertion with VectorStorageBuilderInterface::addFeature(const std::string& name, const std::vector<float>& vec) starting from 0)
	virtual Index featureIndex( const std::string& name) const=0;

	/// \brief Calculate a value between 0.0 and 1.0 representing the similarity of two vectors according to the model used by this storage
	/// \param[in] v1 first input vector
	/// \param[in] v2 second input vector
	/// \return the similarity measure
	virtual double vectorSimilarity( const std::vector<float>& v1, const std::vector<float>& v2) const=0;

	/// \brief Get the number of feature vectors defined
	/// \return the number of features
	virtual unsigned int nofFeatures() const=0;

	/// \brief Get the configuration of this model
	/// \return the configuration string
	virtual std::string config() const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily.
	virtual void close()=0;
};

}//namespace
#endif


