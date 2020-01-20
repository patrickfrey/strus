/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Ranking value assigned to a field (ordinal position range)
/// \file weightedField.hpp
#ifndef _STRUS_WEIGHTED_FIELD_HPP_INCLUDED
#define _STRUS_WEIGHTED_FIELD_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/base/math.hpp"
#include <utility>
#include <limits>

namespace strus {

/// \class WeightedField
/// \brief Ranking value assigned to a field (ordinal position range)
class WeightedField
{
public:
	/// \brief Default constructor
	WeightedField()
		:m_field(),m_weight(0.0){}
	/// \brief Copy constructor
	WeightedField( const WeightedField& o)
		:m_field(o.m_field),m_weight(o.m_weight){}
	/// \brief Constructor
	WeightedField( const strus::IndexRange& field_, double weight_)
		:m_field(field_),m_weight(weight_){}
	WeightedField( strus::Index start_, strus::Index end_, double weight_)
		:m_field(start_,end_),m_weight(weight_){}
	WeightedField& operator=( const WeightedField& o)
		{m_field=o.m_field; m_weight=o.m_weight; return *this;}

	/// \brief Get the document number of the result
	const IndexRange& field() const				{return m_field;}
	/// \brief Get the accumulated weight of the ranking of the result
	double weight() const					{return m_weight;}

	/// \brief Comparison for sorting
	bool operator < ( const WeightedField& o) const
	{
		double diff = m_weight - o.m_weight;
		if (strus::Math::abs( diff) < std::numeric_limits<double>::epsilon())
		{
			return m_field < o.m_field;
		}
		else
		{
			return (diff < 0.0);
		}
	}
	/// \brief Comparison for sorting
	bool operator > ( const WeightedField& o) const
	{
		double diff = m_weight - o.m_weight;
		if (strus::Math::abs( diff) < std::numeric_limits<double>::epsilon())
		{
			return m_field > o.m_field;
		}
		else
		{
			return (diff > 0.0);
		}
	}

private:
	strus::IndexRange m_field;	///< ranking field (ordinal position range)
	double m_weight;		///< ranking weight assigned
};

}//namespace
#endif

