/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for contructing some document data for an update in the storage
/// \file "storageDocumentUpdateInterface.hpp"
#ifndef _STRUS_STORAGE_DOCUMENT_UPDATE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_UPDATE_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/index.hpp"
#include "strus/numericVariant.hpp"

namespace strus
{

/// \class StorageDocumentUpdateInterface
/// \brief Object to declare the items for an update of a document in the storage
class StorageDocumentUpdateInterface
{
public:
	/// \brief Destructor
	virtual ~StorageDocumentUpdateInterface(){}

	/// \brief Define a meta data element of the document by name
	/// \note Meta data are used for query restrictions and for document weights in query result ranking
	/// \note Document meta data have to be declared in advance when creating the storage or with an alter metadata table command when no clients are running on this storage.
	/// \param[in] name_ name of the existing meta data element
	/// \param[in] value_ value of the element
	virtual void setMetaData(
			const std::string& name_,
			const NumericVariant& value_)=0;

	/// \brief Define an attribute (string) of the document
	/// \note Attributes are used for summarization in a query result
	/// \param[in] name_ name of the attribute
	/// \param[in] value_ value of the document attribute
	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_)=0;

	/// \brief Delete an attribute (string) of the document
	/// \note Attributes are used for summarization in a query result
	/// \param[in] name_ name of the attribute
	virtual void clearAttribute(
			const std::string& name_)=0;

	/// \brief Allow a user to access the document
	/// \param[in] username_ name of the user to allow access
	/// \remark The access is only retriected for search, not for the admin interfaces for inspecting, deleting and updating a document
	/// \remark The access control is a very primitive implementation and just designed as filter in the search index.
	/// \note Adding no user means that nobody can search for the document if access control is enabled
	/// \note The storage has to be created with "acl=yes" for enabling access control
	virtual void setUserAccessRight(
			const std::string& username_)=0;

	/// \brief Disallow a user to access the document
	/// \param[in] username_ name of the user to allow access
	virtual void clearUserAccessRight(
			const std::string& username_)=0;

	/// \brief Clear the user rights for all users
	virtual void clearUserAccessRights()=0;

	/// \brief Closing the document definition
	virtual void done()=0;
};

}//namespace
#endif


