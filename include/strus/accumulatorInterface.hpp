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
#ifndef _STRUS_ACCUMULATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_ACCUMULATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus
{

class AccumulatorInterface
{
public:
	virtual ~AccumulatorInterface(){}

	virtual bool nextRank( Index& docno_, int& state_, double& weigth_)
	{
		// Default implementation:
		docno_ = skipDoc( docno_);
		state_ = 0;
		if (!docno_) return 0;
		weigth_ = weight();
		return true;
	}

	virtual Index skipDoc( const Index& docno)=0;
	virtual double weight()=0;

	virtual AccumulatorInterface* copy() const=0;
};

}//namespace
#endif

