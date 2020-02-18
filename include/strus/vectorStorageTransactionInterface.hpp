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
#include "strus/storage/index.hpp"
#include "strus/storage/wordVector.hpp"
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

	/// \brief Define a vector for a feature for later retrieval
	/// \param[in] type of the feature associated with the vector
	/// \param[in] feat name of the feature associated with the vector
	/// \param[in] vec vector to define for this feature
	virtual void defineVector( const std::string& type, const std::string& feat, const WordVector& vec)=0;

	/// \brief Define a feature without vector
	/// \param[in] type of the feature to add
	/// \param[in] feat name of the feature to add
	virtual void defineFeature( const std::string& type, const std::string& feat)=0;

	/// \brief Clear all vectors,types and feature names in the storage
	virtual void clear()=0;

	/// \brief Ensure the persistent storage of the features added with addFeature(const std::string&,const WordVector&) till now
	/// \return true on success, false if failed
	virtual bool commit()=0;

	/// \brief Rollback of the transaction, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


