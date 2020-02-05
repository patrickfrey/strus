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
#include "strus/base/enable_if.hpp"
#include "strus/base/type_traits.hpp"

namespace strus
{

template <typename ValueType>
struct WeightedValue
{
	template <typename T>
	static typename strus::enable_if<
			strus::is_arithmetic<T>::value || strus::is_pointer<T>::value
			,T>::type getNullValueInitialization() {return (T)0;}
	
	template <typename T>
	static typename strus::enable_if<
			!strus::is_arithmetic<T>::value && !strus::is_pointer<T>::value
			,T>::type getNullValueInitialization() {return T();}

	double weight;
	ValueType value;

	WeightedValue()
		:weight(0.0),value(getNullValueInitialization<ValueType>()){}
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


