/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	/// \param[in] docno document number to seek
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


