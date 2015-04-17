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
#ifndef _STRUS_STORAGE_DOCUMENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_DOCUMENT_INTERFACE_HPP_INCLUDED
#include <string>
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"

namespace strus
{

/// \class StorageDocumentInterface
/// \brief Object to declare all items for one insert/update of a document in the storage
class StorageDocumentInterface
{
public:
	/// \brief Destructor
	virtual ~StorageDocumentInterface(){}

	/// \brief Add one occurrence of a term in the document for retrieval
	/// \param[in] type_ type name of the term
	/// \param[in] value_ value string of the term
	/// \param[in] position_ position of the term in the document
	/// \remark Weights accumulated for each type,value,position tuple
	virtual void addSearchIndexTerm(
			const std::string& type_,
			const std::string& value_,
			const Index& position_)=0;

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
			const ArithmeticVariant& value_)=0;

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
	/// \remark The access control is a very primitive implementation and just designed as filter in the search index. You should not rely on this mechanism alone.
	/// \note Adding no user means that nobody can search for the document if access control is enabled
	/// \note The storage has to be created with "acl=yes" for enabling access control
	virtual void setUserAccessRight(
			const std::string& username_)=0;

	/// \brief Closing the document definition
	virtual void done()=0;
};

}//namespace
#endif


