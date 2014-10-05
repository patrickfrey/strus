/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_ACCUMULATOR_OPERATORS_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_OPERATORS_HPP_INCLUDED
#include "accumulator/accumulatorOperatorTemplate.hpp"
#include "strus/index.hpp"

namespace strus
{

class MatchSum
{
public:
	float operator()( IteratorInterface& itr)
	{
		return 1.0;
	}
};

/// \class AccumulatorOperatorSum_td
/// \brief Accumulator for calculating the sum of maching terms for each document
class AccumulatorOperatorSum_td
	:public AccumulatorOperatorTemplate<MatchSum>
{
public:
	AccumulatorOperatorSum_td( const std::vector<IteratorReference>& arg_)
		:AccumulatorOperatorTemplate(arg_){}

	virtual double cur_weight()
	{
		return (double)m_matches.size();
	}

	virtual double min_weight()
	{
		return 1.0;
	}

	virtual double max_weight()
	{
		return (double)m_arg.size();
	}
};



/// \class AccumulatorOperatorNormSum_td
/// \brief Accumulator for calculating the normalized [0 .. 1] sum of maching terms for each document
class AccumulatorOperatorNormSum_td
	:public AccumulatorOperatorTemplate<MatchSum>
{
public:
	AccumulatorOperatorNormSum_td( const std::vector<IteratorReference>& arg_)
		:AccumulatorOperatorTemplate(arg_){}

	virtual double cur_weight()
	{
		if (m_arg.empty())
		{
			return 0.0;
		}
		else
		{
			return (double)m_matches.size() / (double)m_arg.size();
		}
	}

	virtual double min_weight()
	{
		if (m_arg.empty())
		{
			return 0.0;
		}
		else
		{
			return (double)1.0 / (double)m_arg.size();
		}
	}

	virtual double max_weight()
	{
		return (double)1.0;
	}
};


class WeightSum
{
public:
	float operator()( IteratorInterface& itr)
	{
		return itr.weight();
	}
};

/// \class AccumulatorOperatorSum_weight
/// \brief Accumulator for calculating the sum of weights for each matching document
typedef AccumulatorOperatorTemplate<WeightSum> AccumulatorOperatorSum_weight;



class FrequencySum
{
public:
	float operator()( IteratorInterface& itr)
	{
		return (float)itr.frequency();
	}
};

typedef AccumulatorOperatorTemplate<FrequencySum> AccumulatorOperatorSum_tf;

}//namespace
#endif

