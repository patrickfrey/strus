/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Weighted document of the query evaluation (result document without attributes)
/// \file weightedDocument.hpp
#ifndef _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#define _STRUS_WEIGHTED_DOCUMENT_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/base/math.hpp"
#include <utility>
#include <limits>

namespace strus {

/// \class WeightedDocument
/// \brief Pure ranking result of a strus query without the attributes
class WeightedDocument
{
public:
	/// \brief Default constructor
	WeightedDocument()
		:m_docno(0),m_weight(0.0){}
	/// \brief Copy constructor
	WeightedDocument( const WeightedDocument& o)
		:m_docno(o.m_docno),m_weight(o.m_weight){}
	/// \brief Constructor
	WeightedDocument( const Index& docno_, double weight_)
		:m_docno(docno_),m_weight(weight_){}
	WeightedDocument& operator=( const WeightedDocument& o)
		{m_docno=o.m_docno; m_weight=o.m_weight; return *this;}

	/// \brief Get the document number of the result
	Index docno() const					{return m_docno;}
	/// \brief Get the accumulated weight of the ranking of the result
	double weight() const					{return m_weight;}

	/// \brief Comparison for sorting
	bool operator < ( const WeightedDocument& o) const
	{
		double diff = m_weight - o.m_weight;
		if (strus::Math::abs( diff) < std::numeric_limits<double>::epsilon())
		{
			return m_docno < o.m_docno;
		}
		else
		{
			return (diff < 0.0);
		}
	}
	/// \brief Comparison for sorting
	bool operator > ( const WeightedDocument& o) const
	{
		double diff = m_weight - o.m_weight;
		if (strus::Math::abs( diff) < std::numeric_limits<double>::epsilon())
		{
			return m_docno > o.m_docno;
		}
		else
		{
			return (diff > 0.0);
		}
	}

private:
	Index m_docno;			///< document number
	double m_weight;		///< accumulated ranking weight
};

}//namespace
#endif

