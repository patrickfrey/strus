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

