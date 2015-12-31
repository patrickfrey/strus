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
#include "strus/postingIteratorInterface.hpp"
#include <string>

namespace strus {

/// \brief Iterator representing an empty set
class NullIterator
	:public PostingIteratorInterface
{
public:
	NullIterator( Index termtypeno, Index termvalueno, const char* termstr);

	virtual ~NullIterator(){}

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const
	{
		return std::vector<const PostingIteratorInterface*>();
	}
	virtual const char* featureid() const
	{
		return m_featureid.c_str();
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

	virtual Index documentFrequency() const
	{
		return 0;
	}
	
	virtual Index docno() const
	{
		return 0;
	}

	virtual Index posno() const
	{
		return 0;
	}

private:
	std::string m_featureid;
};

}
#endif
