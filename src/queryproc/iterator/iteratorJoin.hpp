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
#ifndef _STRUS_ITERATOR_JOIN_HPP_INCLUDED
#define _STRUS_ITERATOR_JOIN_HPP_INCLUDED
#include "strus/iteratorInterface.hpp"
#include <stdexcept>
#include <limits>

namespace strus
{

/// \brief Iterator interface for join iterators with the common part implemented
class IteratorJoin
	:public IteratorInterface
{
public:
	virtual ~IteratorJoin(){}

	virtual const std::string& featureid() const=0;
	virtual Index skipDoc( const Index& docno)=0;
	virtual Index skipPos( const Index& firstpos)=0;

	virtual unsigned int frequency()
	{
		Index idx=0;
		unsigned int rt = 0;
		for (;0!=(idx=skipPos( idx))
			&& rt < (unsigned int)std::numeric_limits<short>::max();
				++idx,++rt){}
		return rt;
	}

	virtual Index documentFrequency()=0;

	virtual float weight() const
	{
		return 1.0;
	}

	virtual IteratorInterface* copy() const=0;
};

}//namespace
#endif


