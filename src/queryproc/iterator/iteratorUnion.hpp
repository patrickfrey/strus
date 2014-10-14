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
#ifndef _STRUS_ITERATOR_UNION_HPP_INCLUDED
#define _STRUS_ITERATOR_UNION_HPP_INCLUDED
#include "iteratorJoin.hpp"
#include "iteratorReference.hpp"

namespace strus
{

class IteratorUnion
	:public IteratorJoin
{
public:
	IteratorUnion( const IteratorUnion& o);
	IteratorUnion( const IteratorReference& first_, const IteratorReference& second_);
	virtual ~IteratorUnion(){}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipPos( const Index& pos_);

	virtual IteratorInterface* copy() const
	{
		return new IteratorUnion( *this);
	}

private:
	Index m_docno;
	IteratorReference m_first;
	IteratorReference m_second;
	bool m_open_first;
	bool m_open_second;
};

}//namespace
#endif


