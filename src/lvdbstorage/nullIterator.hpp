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
#ifndef _STRUS_LVDB_NULL_ITERATOR_HPP_INCLUDED
#define _STRUS_LVDB_NULL_ITERATOR_HPP_INCLUDED
#include "strus/iteratorInterface.hpp"

namespace strus {

/// \brief Iterator representing an empty set
class NullIterator
	:public IteratorInterface
{
public:
	NullIterator( Index termtypeno, Index termvalueno, const char* termstr);

	NullIterator( const NullIterator& o)
		:m_featureid(o.m_featureid){}

	virtual ~NullIterator(){}

	virtual std::vector<IteratorInterface*> subExpressions( bool positive)
	{
		return std::vector<IteratorInterface*>();
	}
	virtual const std::string& featureid() const
	{
		return m_featureid;
	}

	virtual Index skipDoc( const Index&)
	{
		return 0;
	}

	virtual Index skipPos( const Index&)
	{
		return 0;
	}

	virtual unsigned int frequency()
	{
		return 0;
	}

	virtual Index documentFrequency()
	{
		return 0;
	}
	
	virtual IteratorInterface* copy() const
	{
		return new NullIterator(*this);
	}

private:
	std::string m_featureid;
};

}
#endif