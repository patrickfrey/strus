/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for iterating on the document numbers a user is allowed to see
/// \file "invAclIteratorInterface.hpp"
#ifndef _STRUS_INVERTED_ACL_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_INVERTED_ACL_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus
{

/// \brief Iterator of sets of documents used to implement ACL access restrictions
class InvAclIteratorInterface
{
public:
	/// \brief Destructor
	virtual ~InvAclIteratorInterface(){}

	/// \brief Return the nearest element with a document number higher than or equal to docno
	/// \param[in] docno minimum document number to seek
	/// \return the minumum upper bound value for docno in the traversed set of documents
	virtual Index skipDoc( const Index& docno)=0;
};

}//namespace
#endif

