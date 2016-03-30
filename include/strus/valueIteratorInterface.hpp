/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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


