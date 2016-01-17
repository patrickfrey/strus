/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for accessing the forward index of the storage
/// \file "forwardIteratorInterface.hpp"
#ifndef _STRUS_FORWARD_INDEX_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>

namespace strus
{

/// \brief Iterator on the forward index mapping occurrencies to the terms inserted
class ForwardIteratorInterface
{
public:
	/// \brief Destructor
	virtual ~ForwardIteratorInterface(){}

	/// \brief Define the document of the items inspected
	/// \param[in] docno minimum document number to seek
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Return the next matching position higher than or equal to firstpos in the current document. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	/// \param[in] firstpos minimum position to seek
	/// \return the matching upper bound position found
	virtual Index skipPos( const Index& firstpos)=0;

	/// \brief Fetch the item at the current position
	/// \return the element string
	virtual std::string fetch()=0;
};

}//namespace
#endif


