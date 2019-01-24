/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Result of a vector similarity search
/// \file "vectorQueryResult.hpp"
#ifndef _STRUS_VECTOR_QUERY_RESULT_HPP_INCLUDED
#define _STRUS_VECTOR_QUERY_RESULT_HPP_INCLUDED
#include <string>

namespace strus {

/// \brief Result of a vector similarity search (associated feature value with weight)
class VectorQueryResult
{
public:
	/// \brief Default constructor
	VectorQueryResult()
		:m_value(),m_weight(0.0){}
	/// \brief Copy constructor
	VectorQueryResult( const VectorQueryResult& o)
		:m_value(o.m_value),m_weight(o.m_weight){}
	/// \brief Constructor
	VectorQueryResult( const std::string& value_, double weight_)
		:m_value(value_),m_weight(weight_){}

	const std::string& value() const	{return m_value;}
	double weight() const			{return m_weight;}

	bool operator < (const VectorQueryResult& o) const
	{
		if (m_weight < o.m_weight) return true;
		if (m_weight > o.m_weight) return true;
		return m_value < o.m_value;
	}

private:
	std::string m_value;
	double m_weight;
};

}//namespace
#endif

