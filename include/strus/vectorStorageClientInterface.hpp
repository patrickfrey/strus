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
#include "strus/wordVector.hpp"
#include "strus/vectorQueryResult.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class VectorStorageTransactionInterface;
/// \brief Forward declaration
class ValueIteratorInterface;
/// \brief Forward declaration
class SentenceLexerInstanceInterface;

/// \brief Interface to a repository for vectors representing word embeddings
class VectorStorageClientInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageClientInterface(){}

	/// \brief Prepare datastructures for calling findSimilar( const WordVector&, int,double,bool)const'
	/// \remark This method does not have to be called, because the structures for search are built implicitely on the first search. It just avoids a massive delay on the first call of find similar.
	/// \param[in] type type of the features to search for
	virtual void prepareSearch( const std::string& type)=0;

	/// \brief Find all features that are within maximum simiarity distance.
	/// \param[in] type type of the features to search for
	/// \param[in] vec vector to search similar features of
	/// \param[in] maxNofResults limits the number of results returned
	/// \param[in] minSimilarity value between 0.0 and 1.0 specifying the minimum similarity a result should have (if LSH weighting is used only values above 0.5 make sense)
	/// \param[in] speedRecallFactor factor bigger than 0.0 used to adjust performance/recall, smaller value = better performance, bigger value = better recall
	/// \remark for the standard LSH implementation a good value is 0.8. In the standard LSH implementation the value describes the accepted difference in bits of a sample, compared with the average error accepted. For example if the accepted difference in bits of the LSH values compared is 400 out of 2048, then the accepted difference for a sample of 64 bits that decides wheter a candidate should be considered further is (1.0 + speedRecallFactor) * 400 / (2048/64)) = (1.0 + speedRecallFactor) * 12.5 bits.
	/// \param[in] realVecWeights true if to calculate the real vector weights and diffs for the best matches and not an approximate value (e.g. the similarity estimation derived from the LSH bits differing)
	virtual std::vector<VectorQueryResult> findSimilar( const std::string& type, const WordVector& vec, int maxNofResults, double minSimilarity, double speedRecallFactor, bool realVecWeights) const=0;

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

	/// \brief Create a lexer for parsing query sentences based on the vector storage
	/// \return the lexer (with ownership)
	/// \remark the returned object is dependent on the vector storage client that created it
	virtual SentenceLexerInstanceInterface* createSentenceLexer() const=0;

	/// \brief Get the configuration of this storage
	/// \return the configuration string
	virtual std::string config() const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily.
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void close()=0;

	/// \brief Do compaction of data.
	/// \remark This method is also called as side effect close
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void compaction()=0;
};

}//namespace
#endif


