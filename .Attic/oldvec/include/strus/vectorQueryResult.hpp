/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result of a closest vector search
/// \file "vectorQueryResult.hpp"
#ifndef _STRUS_VECTOR_QUERY_RESULT_HPP_INCLUDED
#define _STRUS_VECTOR_QUERY_RESULT_HPP_INCLUDED
#include <limits>

namespace strus {

/// \brief Result of a vector search (associated feature number with weight)
class VectorQueryResult
{
public:
	/// \brief Default constructor
	VectorQueryResult()
		:m_featidx(0),m_weight(0.0){}
	/// \brief Copy constructor
	VectorQueryResult( const VectorQueryResult& o)
		:m_featidx(o.m_featidx),m_weight(o.m_weight){}
	/// \brief Constructor
	VectorQueryResult( const Index& featidx_, double weight_)
		:m_featidx(featidx_),m_weight(weight_){}

	Index featidx() const			{return m_featidx;}
	double weight() const			{return m_weight;}

	void setWeight( double weight_)		{m_weight = weight_;}

	bool operator < ( const VectorQueryResult& o) const
	{
		if (m_weight + std::numeric_limits<double>::epsilon() < o.m_weight) return true;
		if (m_weight - std::numeric_limits<double>::epsilon() > o.m_weight) return false;
		return m_featidx < o.m_featidx;
	}
	bool operator > ( const VectorQueryResult& o) const
	{
		if (m_weight + std::numeric_limits<double>::epsilon() < o.m_weight) return false;
		if (m_weight - std::numeric_limits<double>::epsilon() > o.m_weight) return true;
		return m_featidx > o.m_featidx;
	}

private:
	Index m_featidx;
	double m_weight;
};

}//namespace
#endif

