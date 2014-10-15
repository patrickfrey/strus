/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#define _STRUS_WEIGHTING_CONSTANT_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/index.hpp"
#include "strus/iteratorInterface.hpp"
#include "weightingIdfBased.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \class WeightingConstant
/// \brief Accumulator for the feature frequency
class WeightingConstant
	:public WeightingIdfBased
{
public:
	WeightingConstant(
			double weight_,
			const StorageInterface* storage_,
			const EstimatedNumberOfMatchesMapR& nofMatchesMap_)
		:WeightingIdfBased(storage_,nofMatchesMap_),m_weight(weight_){}
	WeightingConstant( const WeightingConstant& o)
		:WeightingIdfBased(o),m_weight(o.m_weight){}

	virtual ~WeightingConstant(){}

	double call( IteratorInterface& itr)
	{
		return m_weight;
	}

	virtual WeightingFunctionInterface* copy() const
	{
		return new WeightingConstant( *this);
	}

private:
	double m_weight;
};

}//namespace
#endif

