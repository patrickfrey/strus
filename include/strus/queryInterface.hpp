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
#ifndef _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include "strus/queryeval/resultDocument.hpp"

namespace strus {

/// \brief Forward declaration
class StorageInterface;

/// \brief Defines a strus retrieval query as tree
class QueryInterface
{
public:
	virtual ~QueryInterface(){}

	virtual void print( std::ostream& out)=0;

	virtual void pushTerm( const std::string& type_, const std::string& value_)=0;
	virtual void pushExpression( const std::string& opname_, std::size_t argc, int range_)=0;
	virtual void defineFeature( const std::string& set_, float weight_=1.0)=0;

	virtual void setMaxNofRanks( std::size_t maxNofRanks_)=0;
	virtual void setMinRank( std::size_t minRank_)=0;
	virtual void setUserName( const std::string& username_)=0;

	virtual std::vector<queryeval::ResultDocument> evaluate(
			const StorageInterface* storage)=0;
};

}//namespace
#endif
