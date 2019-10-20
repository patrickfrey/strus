/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the ACL reader interface
/// \file "aclReader.hpp"
#ifndef _STRUS_ACL_READER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_ACL_READER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/aclReaderInterface.hpp"
#include "strus/reference.hpp"
#include <map>

namespace strus
{

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface for accessing document attributes from a strus storage
class AclReader
	:public AclReaderInterface
{
public:
	AclReader( const StorageClient* storage_, ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~AclReader(){}

	/// \brief Move the ACL reader context to a specific document
	/// \param[in] docno the number of the document to move the context to
	virtual void skipDoc( const Index& docno_);

	/// \brief Get the names of all users that are attached to this document, allowed to read the document
	/// \return user names
	virtual std::vector<std::string> getReadAccessList() const;

private:
	const StorageClient* m_storage;				///< storage handle
	std::map<Index,std::string> m_usermap;			///< map internal userno to user name
	Index m_docno;						///< current document number
	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};
}//namespace
#endif

