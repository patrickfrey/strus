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
/// \brief Interface for introspection of string values stored in the storage
/// \file "valueIteratorInterface.hpp"
#ifndef _STRUS_VALUE_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_VALUE_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \class ValueIteratorInterface
/// \brief Structure that represents a set of strings as iterator.
class ValueIteratorInterface
{
public:
	virtual ~ValueIteratorInterface(){}

	/// \brief Set the next key to find
	/// \param[in] value pointer key string to seek
	/// \param[in] size sizeof of value in bytes
	virtual void skip( const char* value, std::size_t size)=0;

	/// \brief Fetch the next N elements
	/// \param[in] maxNofElements maximum number of elements to fetch
	/// \return list of next values
	/// \note the iterator position is on the element following the last element returned or at EOF
	virtual std::vector<std::string> fetchValues( std::size_t maxNofElements)=0;
};

}//namespace
#endif


