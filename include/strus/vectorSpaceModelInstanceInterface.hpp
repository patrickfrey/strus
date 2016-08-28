/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for vector space model operations needed for efficient similar vector retrieval in the strus storage
#ifndef _STRUS_VECTOR_SPACE_MODEL_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_INSTANCE_INTERFACE_HPP_INCLUDED
#include <bitset>
#include <vector>

namespace strus {

/// \brief Instance Interface for vector space model operations needed for efficient similar vector retrieval in the strus storage
class VectorSpaceModelInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~VectorSpaceModelInstanceInterface(){}

	/// \brief Initialize the model
	/// \param[in] dim dimension of the model vector space
	/// \param[in] variations number of mappings used in similarity hash calculation = number of bits a similarity hash has
	virtual void initialize( std::size_t dim, std::size_t variations)=0;

	/// \brief Initialize the model from a dump
	virtual void load( const std::vector<double>& serialization)=0;

	/// \brief Get the serialization of the model that can be reloaded with 'load(const std::vector<double>&)'
	/// \return the serialization
	virtual std::vector<double> serialize() const=0;

	/// \brief Return a hash value with the property that two similar vectors (cosine similarity) have with a high probability only a few different bits, e.g. the bitcount of the XOR of the two vectors is very small.
	/// \param[in] vec vector to calculate the hash from
	/// \return the result of the hash function
	/// \remark This method requires that either 'initialize( std::size_t, std::size_t)' or 'load(const std::vector<double>&)' is called before
	virtual std::vector<bool> getSimHash( const std::vector<double>& vec) const=0;

	/// \brief Calculate the cosine similarity of two vectors.
	/// \param[in] v1 first input vector
	/// \param[in] v2 second input vector
	/// \return the similarity value (1.0 for equal, 0.0 for orthogonal)
	/// \remark This method requires that either 'initialize( std::size_t, std::size_t)' or 'load(const std::vector<double>&)' is called before
	virtual double calculateCosineSimilarity( const std::vector<double>& v1, const std::vector<double>& v2) const=0;
};


