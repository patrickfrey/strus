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
#include "strus/wordVector.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class VectorStorageSearchInterface;
/// \brief Forward declaration
class VectorStorageTransactionInterface;
/// \brief Forward declaration
class ValueIteratorInterface;

/// \brief Interface to a repository for vectors representing word embeddings
class VectorStorageClientInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageClientInterface(){}

	/// \brief Create a search interface for scanning the vectors for similarity to a vector
	/// \param[in] type type of the features to search for
	/// \param[in] indexPart which part (index starting from 0) of nofParts pieces to build searcher for (possibility to split search into multiple instances/threads)
	/// \param[in] nofParts how many parts exist (number starting from 1) to select with indexPart (possibility to split into multiple instances)
	/// \return the search interface (with ownership)
	virtual VectorStorageSearchInterface* createSearcher( const std::string& type, int indexPart, int nofParts) const=0;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface (with ownership)
	/// \note this function is thread safe, multiple concurrent transactions are allowed 
	virtual VectorStorageTransactionInterface* createTransaction()=0;

	/// \brief Get the list of types defined
	/// \return the list
	virtual std::vector<std::string> types() const=0;

	/// \brief Create an iterator on the feature values inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createFeatureValueIterator() const=0;

	/// \brief Get the list of types assigned to a specific featureValue
	/// \param[in] featureValue feature value to get the types assigned to
	/// \return the list of types assigned to 'featureValue'
	virtual std::vector<std::string> featureTypes( const std::string& featureValue) const=0;

	/// \brief Get the number of vectors defined for the features of a type
	/// \param[in] type name of the type
	/// \return the number of vectors
	virtual int nofVectors( const std::string& type) const=0;

	/// \brief Get the vector assigned to a feature value
	/// \param[in] type type of the feature
	/// \param[in] featureValue value of the feature
	/// \return the vector assinged to this feature
	virtual WordVector featureVector( const std::string& type, const std::string& featureValue) const=0;

	/// \brief Calculate a value between 0.0 and 1.0 representing the similarity of two vectors
	/// \param[in] v1 first input vector
	/// \param[in] v2 second input vector
	/// \return the similarity measure
	virtual double vectorSimilarity( const WordVector& v1, const WordVector& v2) const=0;

	/// \brief Calculate the normalized vector representation of the argument vector
	virtual WordVector normalize( const WordVector& vec) const=0;

	/// \brief Get the configuration of this storage
	/// \return the configuration string
	virtual std::string config() const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily.
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void close()=0;
};

}//namespace
#endif


