/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_UTILS_WEIGHTED_VALUE_HPP_INCLUDED
#define _STRUS_QUERYPROC_UTILS_WEIGHTED_VALUE_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus
{

template <typename ValueType, ValueType NullValue = 0>
struct WeightedValue
{
	double weight;
	ValueType value;

	WeightedValue()
		:weight(0.0),value(NullValue){}
	WeightedValue( double weight_, const ValueType& value_)
		:weight(weight_),value(value_){}
	WeightedValue( const WeightedValue& o)
		:weight(o.weight),value(o.value){}

	bool operator < (const WeightedValue& o) const
	{
		return (strus::Math::isequal( weight, o.weight))
			? value < o.value
			: weight < o.weight;
	}
	bool operator > (const WeightedValue& o) const
	{
		return (strus::Math::isequal( weight, o.weight))
			? value > o.value
			: weight > o.weight;
	}
};

}//namespace
#endif


