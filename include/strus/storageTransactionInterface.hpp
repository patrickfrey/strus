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
/// \brief Interface for a transaction on the storage
/// \file "storageTransactionInterface.hpp"
#ifndef _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/storageDocumentInterface.hpp"
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class StorageDocumentUpdateInterface;

/// \class StorageTransactionInterface
/// \brief Object to declare all items for one insert/update of a document in the storage
class StorageTransactionInterface
{
public:
	/// \brief Destructor
	/// \remark Expected to do an implicit rollback, if neither 'commit()' or 'rollback' was called
	virtual ~StorageTransactionInterface(){}

	/// \brief Create one document to be inserted/replaced within this transaction
	/// \param[in] docid_ identifier of the document
	/// \param[in] docno_ document number of the document
	/// \remark If the document number (second argument) is defined by the client, then the server is not called for a document number (no synchronization needed) and documents close to each other can get adjacent document numbers even if the transaction is done in parallel with another.
	/// \return the document object
	virtual StorageDocumentInterface*
		createDocument(
			const std::string& docid_,
			const Index& docno_=0)=0;

	/// \brief Create an interface for a document to be updated within this transaction
	/// \param[in] docno_ document number of the document (StorageClientInterface::documentNumber( const std::string&))
	/// \return the document updater object
	virtual StorageDocumentUpdateInterface*
		createDocumentUpdate(
			const Index& docno_)=0;

	/// \brief Declare a document to be removed from the storage within this transaction
	/// \param[in] docid document identifier (URI)
	virtual void deleteDocument(
			const std::string& docid)=0;

	/// \brief Declare the access rights of a user to any document to be removed from the storage within this transaction
	/// \param[in] username user name
	virtual void deleteUserAccessRights(
			const std::string& username)=0;

	/// \brief Update of meta data for a specific document (without creating it)
	/// \param[in] docno document number
	/// \param[in] varname meta data element name to update
	/// \param[in] value new value of this meta data element
	virtual void updateMetaData(
			const Index& docno,
			const std::string& varname,
			const ArithmeticVariant& value)=0;

	/// \brief Insert all documents and executes all commands defined in the transaction or none if one operation fails
	virtual void commit()=0;

	/// \brief Rollback of the transaction, no changes made
	virtual void rollback()=0;
};

}//namespace
#endif


