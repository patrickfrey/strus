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
#include "strus/weightedField.hpp"
#include "strus/base/math.hpp"
#include <utility>
#include <limits>

namespace strus {

/// \class WeightedDocument
/// \brief Pure ranking result of a strus query without summaries
class WeightedDocument
{
public:
	/// \brief Default constructor
	WeightedDocument()
		:m_docno(0),m_field(),m_weight(0.0){}
	/// \brief Copy constructor
	WeightedDocument( const WeightedDocument& o)
		:m_docno(o.m_docno),m_field(o.m_field),m_weight(o.m_weight){}
	/// \brief Constructor
	WeightedDocument( strus::Index docno_, const strus::IndexRange& field_, double weight_)
		:m_docno(docno_),m_field(field_),m_weight(weight_){}
	WeightedDocument& operator=( const WeightedDocument& o)
		{m_docno=o.m_docno; m_field=o.m_field; m_weight=o.m_weight; return *this;}

	/// \brief Get the document number of this weighted document
	Index docno() const					{return m_docno;}
	/// \brief Get the field weighted
	const IndexRange& field() const				{return m_field;}
	/// \brief Get the weight of this weighted document
	double weight() const					{return m_weight;}

	/// \brief Set the weight of this weighted document
	void setWeight( double weight_)				{m_weight = weight_;}

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
	strus::IndexRange m_field;	///< ranking field (ordinal position range)
	double m_weight;		///< accumulated ranking weight
};

}//namespace
#endif

