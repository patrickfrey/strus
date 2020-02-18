/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface (readonly) for accessing document access control list (read)
/// \file "aclReaderInterface.hpp"
#ifndef _STRUS_ACL_READER_INTERFACE_HPP_INCLUDED
#define _STRUS_ACL_READER_INTERFACE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for accessing document attributes from a strus storage
class AclReaderInterface
{
public:
	/// \brief Destructor
	virtual ~AclReaderInterface(){}

	/// \brief Move the ACL reader context to a specific document
	/// \param[in] docno the number of the document to move the context to
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Get the names of all users that are attached to this document, allowed to read the document
	/// \return user names
	virtual std::vector<std::string> getReadAccessList() const=0;
};
}//namespace
#endif

