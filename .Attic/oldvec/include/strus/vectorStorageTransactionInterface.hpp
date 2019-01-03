/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Vector repository transaction implementation
#ifndef _STRUS_VECTOR_STORGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>
#include <string>
namespace strus {

/// \brief Interface for building a repository of vectors with classifiers to map them to discrete features.
/// \remark This interface has the transaction context logically enclosed in the object, though use in a multithreaded context does not make much sense. Thread safety of the interface is guaranteed, but not the performance in a multithreaded context. It is thought as class that internally makes heavily use of multithreading, but is not thought to be fed by mutliple threads.
class VectorStorageTransactionInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageTransactionInterface(){}

	/// \brief Add a feature to the repository for later retrieval
	/// \param[in] name name of the feature associated with the vector to add
	/// \param[in] vec vector to add
	virtual void addFeature( const std::string& name, const std::vector<float>& vec)=0;

	/// \brief Define a relation element 
	/// \param[in] conceptClass name of the concept class to distiguish different relations
	/// \param[in] featidx index of the feature (see 'VectorStorageClientInterface::featureIndex( const std::string&) const')
	/// \param[in] conidx index of the concept
	virtual void defineFeatureConceptRelation( const std::string& conceptClass, const Index& featidx, const Index& conidx)=0;

	/// \brief Ensure the persistent storage of the features added with addFeature(const std::string&,const std::vector<float>&) till now
	/// \return true on success, false if failed
	virtual bool commit()=0;

	/// \brief Rollback of the transaction, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


