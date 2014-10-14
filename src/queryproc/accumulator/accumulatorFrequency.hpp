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
#ifndef _STRUS_ACCUMULATOR_FREQUENCY_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_FREQUENCY_HPP_INCLUDED
#include "strus/accumulatorInterface.hpp"
#include "strus/index.hpp"
#include "iteratorReference.hpp"
#include <limits>
#include <vector>

namespace strus
{

/// \class AccumulatorFrequency
/// \brief Accumulator for the feature frequency
class AccumulatorFrequency
	:public AccumulatorInterface
{
public:
	explicit AccumulatorFrequency( const IteratorInterface& itr_)
		:m_itr(itr_.copy(),){}
	AccumulatorFrequency( const AccumulatorFrequency& o)
		:m_itr(o.m_itr){}

	virtual ~AccumulatorFrequency(){}

	virtual AccumulatorInterface* copy() const
	{
		return new AccumulatorFrequency( *m_itr);
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_itr->skipDoc( docno_);
	}

	virtual double weight()
	{
		return static_cast<double>( m_itr->frequency());
	}

private:
	IteratorReference m_itr;		///< input occurrencies to scan for results
};

}//namespace
#endif

