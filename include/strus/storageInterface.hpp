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
#ifndef _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_INTERFACE_HPP_INCLUDED
#include <string>
#include <vector>
#include "strus/index.hpp"
#include "strus/statCounterValue.hpp"

namespace strus
{

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class DocnoIteratorInterface;
/// \brief Forward declaration
class StorageTransactionInterface;
/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class AttributeReaderInterface;

/// \brief Interface of a strus IR storage
class StorageInterface
{
public:
	/// \brief Destructor
	/// \remark Calls close but ignores errors there silently
	virtual ~StorageInterface(){}

	/// \brief Close the storage and throw on error
	/// \remark Call this function before the destructor if you want to catch errors in the close
	virtual void close(){};

	/// \brief Create an iterator on the occurrencies of a term in the storage
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference to be disposed with delete
	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value) const=0;

	/// \brief Create a viewer to inspect the term stored values with the forward index of the storage
	/// \param[in] type type name of the term to be inspected
	/// \return the created viewer reference to be disposed with delete
	virtual ForwardIteratorInterface*
		createForwardIterator(
			const std::string& type) const=0;

	/// \brief Create a an iterator on the numbers of documents a specified user is allowed to see
	/// \param[in] username name of the user
	/// \return the iterator on the documents or NULL, if there is no access control enabled
	/// \note The storage has to be created access control enabled
	virtual DocnoIteratorInterface*
		createInvertedAclIterator(
			const std::string& username) const=0;

	/// \brief Get the number of documents inserted into the collection
	/// \return the number of documents
	virtual Index nofDocumentsInserted() const=0;

	/// \brief Get the highest document number used in the collection
	/// \return the document number
	virtual Index maxDocumentNumber() const=0;

	/// \brief Get the internal document number
	/// \param[in] docid document id of the document inserted
	virtual Index documentNumber( const std::string& docid) const=0;

	/// \brief Create an interface to access items of document metadata
	/// \param[in] varname variable name identifying the metadata attribute
	/// \return the interface to access document metadata
	virtual MetaDataReaderInterface* createMetaDataReader() const=0;

	/// \brief Create an interface to access attributes attached to documents for representation
	/// \return the interface to access document attributes
	virtual AttributeReaderInterface* createAttributeReader() const=0;

	/// \brief Allocate a range of document numbers to be used for documents known to be new in transactions
	/// \param[in] nofDocuments number of document numbers to allocate
	/// \return the first document number of the allocated range
	virtual Index allocDocnoRange( std::size_t nofDocuments)=0;

	/// \brief Create an insert/update transaction object
	/// \param[in] docid document identifier (URI)
	/// \return the created inserter reference to be disposed with delete
	virtual StorageTransactionInterface* createTransaction()=0;

	/// \brief Create an interface to verify, if the contents of a document are inserted correctly into the storage. The checking is invoked by calling the StorageDocumentInterface::done() method after the definition of all elements.
	/// \param[in] docid identifier (URI) of the document to check
	/// \param[in] logfilename Where to log checking failures ("-" for stdout)
	/// \return the created document reference to be disposed with delete
	virtual StorageDocumentInterface* createDocumentChecker(
			const std::string& docid,
			const std::string& logfilename) const=0;

	/// \brief Get some statistics (counters) of the storage
	virtual std::vector<StatCounterValue> getStatistics() const=0;
};

}//namespace
#endif


