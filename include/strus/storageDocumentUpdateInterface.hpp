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
#include "strus/storage/index.hpp"
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

	/// \brief Add one occurrence of a term to the search index excluding all features of this type previously added in another update context.
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ ordinal position (term count position) of the term in the document
	/// \note All search index terms of this type that are not readded with this document update get removed
	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_)=0;

	/// \brief Add a structure (relation of ordinal position ranges) in the the document for retrieval
	/// \param[in] struct_ structure type name
	/// \param[in] source_ position range of the relation source in the document
	/// \param[in] sink_ position range of the relation sink in the document
	/// \remark Source index ranges must not overlap, sink index ranges must not overlap
	/// \note All search index structures of this type that are not readded with this document update get removed
	virtual void addSearchIndexStructure(
			const std::string& struct_,
			const IndexRange& source_,
			const IndexRange& sink_)=0;

	/// \brief Add one occurrence of a term to the forward index excluding all features of this type previously added in another update context.
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ position of the term in the document
	/// \remark Only one type,value pair allowed at one position
	/// \note All forward index terms of this type that are not readded with this document update get removed
	virtual void addForwardIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_)=0;

	/// \brief Clear all previous occurrencies of a term type in a document in the search index
	/// \param[in] type_ type name of the terms to erase
	/// \note The call of this function happens implicitely, if one or more features of this type are added to the update
	virtual void clearSearchIndexTerm(
			const std::string& type_)=0;

	/// \brief Clear all structures previously declared for a document in the search index
	/// \note The call of this function happens implicitely, if one or more structures are added to the update
	virtual void clearSearchIndexStructures()=0;

	/// \brief Clear all previous occurrencies of a term type in a document in the forward index
	/// \param[in] type_ type name of the terms to erase
	/// \note The call of this function happens implicitely, if one or more features of this type are added to the update
	virtual void clearForwardIndexTerm(
			const std::string& type_)=0;

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


