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
#ifndef _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_EVAL_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>
#include <iostream>

namespace strus
{
/// \brief Forward declaration
class QueryInterface;

/// \brief Defines a program for evaluating a query
class QueryEvalInterface
{
public:
	/// \brief Destructor
	virtual ~QueryEvalInterface(){}

	/// \brief Create a new query
	virtual QueryInterface* createQuery() const=0;

	/// \brief Print the internal representation of this query program to 'out'
	/// \param[out] out stream to print the program to
	virtual void print( std::ostream& out) const=0;
};

}//namespace
#endif

