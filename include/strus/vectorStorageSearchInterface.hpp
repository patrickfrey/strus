/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for searching vectors. Separated from client interface because it requires linear scanning of candidates in memory because of the nature of the problem.
#ifndef _STRUS_VECTOR_STORGE_SEARCH_INTERFACE_HPP_INCLUDED
#define _STRUS_VECTOR_STORGE_SEARCH_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>

namespace strus {

/// \brief Interface to repository for vectors and a feature concept relation model previously created with a builder
class VectorStorageSearchInterface
{
public:
	/// \brief Destructor
	virtual ~VectorStorageSearchInterface(){}

	/// \brief Weighted feature number as result of search
	class Result
	{
	public:
		/// \brief Default constructor
		Result()
			:m_featidx(0),m_weight(0.0){}
		/// \brief Copy constructor
		Result( const Result& o)
			:m_featidx(o.m_featidx),m_weight(o.m_weight){}
		/// \brief Constructor
		Result( const Index& featidx_, double weight_)
			:m_featidx(featidx_),m_weight(weight_){}

		Index featidx() const			{return m_featidx;}
		double weight() const			{return m_weight;}

		void setWeight( double weight_)		{m_weight = weight_;}

		bool operator < ( const Result& o) const
		{
			return (m_weight == o.m_weight) ? m_featidx < o.m_featidx : m_weight < o.m_weight;
		}
		bool operator > ( const Result& o) const
		{
			return (m_weight == o.m_weight) ? m_featidx > o.m_featidx : m_weight > o.m_weight;
		}

	private:
		Index m_featidx;
		double m_weight;
	};

	/// \brief Find all features that are within maximum simiarity distance of the model (or at least try to with best effort, if the model is probabilistic).
	/// \param[in] vec vector to calculate the features from
	/// \param[in] maxNofResults limits the number of results returned
	virtual std::vector<Result> findSimilar( const std::vector<double>& vec, unsigned int maxNofResults) const=0;

	/// \brief Explicit close of the database access, if database was used
	/// \note this function is useful in interpreter context where a garbagge collection may delay the deletion of an object
	virtual void close();
};

}//namespace
#endif

