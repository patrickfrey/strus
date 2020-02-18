/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for contructing a document for inserting it into the storage
/// \file "storageDocumentInterface.hpp"
#ifndef _STRUS_STORAGE_DOCUMENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/storage/index.hpp"
#include "strus/numericVariant.hpp"

namespace strus
{

/// \class StorageDocumentInterface
/// \brief Object to declare all items for one insert/replace of a document in the storage
class StorageDocumentInterface
{
public:
	/// \brief Destructor
	virtual ~StorageDocumentInterface(){}

	/// \brief Add one occurrence of a term in the document for retrieval
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ position of the term in the document
	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_)=0;

	/// \brief Add a structure (relation of ordinal position ranges) in the the document for retrieval
	/// \param[in] struct_ structure type name
	/// \param[in] source_ position range of the relation source in the document
	/// \param[in] sink_ position range of the relation sink in the document
	/// \remark Source index ranges must not overlap, sink index ranges must not overlap
	virtual void addSearchIndexStructure(
			const std::string& struct_,
			const IndexRange& source_,
			const IndexRange& sink_)=0;

	/// \brief Add one occurrence of a term to the forward index for summarization of the document
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ position of the term in the document
	/// \remark Only one type,value pair allowed at one position
	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_)=0;

	/// \brief Define a meta data element of the document by name
	/// \note Meta data are used for query restrictions and for document weights in query result ranking
	/// \note Document meta data have to be declared in advance when creating the storage or with an alter metadata table command when no clients are running on this storage.
	/// \param[in] name_ name of the existing meta data element
	/// \param[in] value_ value of the element
	virtual void setMetaData(
			const std::string& name_,
			const NumericVariant& value_)=0;

	/// \brief Define a string attribute of the document
	/// \note Attributes are used for summarization in a query result
	/// \param[in] name_ name of the attribute
	/// \param[in] value_ value of the document attribute
	virtual void setAttribute(
			const std::string& name_,
			const std::string& value_)=0;

	/// \brief Allow a user to access the document
	/// \param[in] username_ name of the user to allow access
	/// \remark The access is only retriected for search, not for the admin interfaces for inspecting, deleting and updating a document
	/// \remark The access control is a very primitive implementation and just designed as filter in the search index.
	/// \note Adding no user means that nobody can search for the document if access control is enabled
	/// \note The storage has to be created with "acl=yes" for enabling access control
	virtual void setUserAccessRight(
			const std::string& username_)=0;

	/// \brief Closing the document definition and transmit it into the transaction context
	virtual void done()=0;
};

}//namespace
#endif


