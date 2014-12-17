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
#ifndef _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_TRANSACTION_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/storageDocumentInterface.hpp"

namespace strus
{

/// \class StorageTransactionInterface
/// \brief Object to declare all items for one insert/update of a document in the storage
class StorageTransactionInterface
{
public:
	/// \brief Destructor that is the doing the rollback too, if commit() was not called before
	virtual ~StorageTransactionInterface(){}

	/// \brief Create one document to be inserted/updated within this transaction
	/// \param[in] docid_ identifier of the document
	/// \param[in] docno_ document number of the document
	/// \remark If the document number (second argument) is defined by the client, then the server is not called for a document number (no synchronization needed) and documents close to each other can get adjacent document numbers even if the transaction is done in parallel with another.
	/// \return the document object
	virtual StorageDocumentInterface*
		createDocument(
			const std::string& docid_,
			const Index& docno=0)=0;

	/// \brief Declare a document to be removed from the storage within this transaction
	/// \param[in] docid document identifier (URI)
	virtual void deleteDocument(
			const std::string& docid)=0;

	/// \brief Declare the access rights of a user to any document to be removed from the storage within this transaction
	/// \param[in] username user name
	virtual void deleteUserAccessRights(
			const std::string& username)=0;

	/// \brief Write the documents created to the storage
	virtual void commit()=0;

	/// \brief Write the documents created to the storage
	virtual void rollback()=0;
};

}//namespace
#endif


