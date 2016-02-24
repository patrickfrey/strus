/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Client interface for accessing a storage (read/write)
/// \file "storageClientInterface.hpp"
#ifndef _STRUS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/termStatistics.hpp"
#include <string>
#include <vector>
#include <ostream>

namespace strus
{

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class DocumentTermIteratorInterface;
/// \brief Forward declaration
class InvAclIteratorInterface;
/// \brief Forward declaration
class ValueIteratorInterface;
/// \brief Forward declaration
class StorageTransactionInterface;
/// \brief Forward declaration
class StorageDocumentInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class StatisticsIteratorInterface;
/// \brief Forward declaration
class StorageDumpInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class AttributeReaderInterface;


/// \brief Interface of a strus IR storage
class StorageClientInterface
{
public:
	/// \brief Destructor
	/// \remark Should call call 'close()' but ignore errors there silently
	virtual ~StorageClientInterface(){}

	/// \brief Create an iterator on the occurrencies of a term in the storage
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference to be disposed with delete by the caller
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

	/// \brief Create an iterator on term occurrencies in documents (support for feature selection)
	/// \param[in] type type name of the term
	/// \return the created iterator reference to be disposed with delete by the caller
	virtual DocumentTermIteratorInterface*
		createDocumentTermIterator(
			const std::string& type) const=0;

	/// \brief Create a an iterator on the numbers of documents a specified user is allowed to see
	/// \param[in] username name of the user
	/// \return the iterator on the documents to be disposed with delete by the caller or NULL, if there is no access control enabled
	/// \note The storage has to be created access control enabled
	virtual InvAclIteratorInterface*
		createInvAclIterator(
			const std::string& username) const=0;

	/// \brief Get the number of documents inserted in this storage instance
	/// \return the number of documents
	virtual Index nofDocumentsInserted() const=0;

	/// \brief Get the number of documents inserted in this storage instance
	/// \param[in] type the term type addressed
	/// \param[in] term the term value addressed
	/// \return the number of documents
	virtual Index documentFrequency(
			const std::string& type,
			const std::string& term) const=0;

	/// \brief Get the highest document number used in this stogage
	/// \return the document number, or 0, if no documents are inserted
	virtual Index maxDocumentNumber() const=0;

	/// \brief Get the local internal document number
	/// \param[in] docid document id of the document inserted
	/// \return the document number or 0, if it does not exist
	virtual Index documentNumber( const std::string& docid) const=0;

	/// \brief Create an iterator on the term types inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createTermTypeIterator() const=0;

	/// \brief Create an iterator on the term value inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createTermValueIterator() const=0;

	/// \brief Create an iterator on the document identifiers inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createDocIdIterator() const=0;

	/// \brief Create an iterator on the user names used in document access restrictions
	/// \return the iterator
	virtual ValueIteratorInterface* createUserNameIterator() const=0;

	/// \brief Enumeration of document statistics
	enum DocumentStatisticsType
	{
		StatNofTerms = 1,			///< number of distinct terms
		StatNofTermOccurrencies = 2		///< number of accumulated dfs (number of terms)
	};

	/// \brief Get one specified element of the documents statistics for a term type
	/// \param[in] docno the local internal document number addressed (return value of documentNumber( const std::string&) const)
	/// \param[in] stat the enumeration value of the statistics to get
	/// \param[in] type the term type addressed
	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const=0;

	/// \brief Create an interface to access items of document metadata
	/// \return the interface to access document metadata to be disposed with delete by the caller
	virtual MetaDataReaderInterface* createMetaDataReader() const=0;

	/// \brief Create an interface to access attributes attached to documents for representation
	/// \return the interface to access document attributes to be disposed with delete by the caller
	virtual AttributeReaderInterface* createAttributeReader() const=0;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface to be disposed with delete by the caller
	virtual StorageTransactionInterface* createTransaction()=0;

	/// \brief Creates an iterator on storage statistics messages for initialization/deregistration
	/// \param[in] sign true = positive, false = negative, means all offsets are inverted and isnew is false too (used for deregistration)
	/// \return the iterator on the statistics message blobs
	virtual StatisticsIteratorInterface* createInitStatisticsIterator( bool sign=true)=0;

	/// \brief Creates an iterator on the storage statistics messages created by updates of this storage
	/// \return the iterator on the statistics message blobs
	virtual StatisticsIteratorInterface* createUpdateStatisticsIterator()=0;

	/// \brief Get the processing message interface for introspecting and packing messages outside the queue context
	/// \return the message processor interface
	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const=0;

	/// \brief Create an interface to verify, if the contents of a document are inserted correctly into the storage. The checking is invoked by calling the StorageDocumentInterface::done() method after the definition of all elements.
	/// \param[in] docid identifier (URI) of the document to check
	/// \param[in] logfilename Where to log checking failures ("-" for stdout)
	/// \return the created document interface to be disposed with delete by the caller
	virtual StorageDocumentInterface* createDocumentChecker(
			const std::string& docid,
			const std::string& logfilename) const=0;

	/// \brief Iterate through all key/value pairs and check their data for validity
	/// \param[out] errorlog stream for reporting errors
	/// \return true, if the check succeeds, false if it fails
	virtual bool checkStorage( std::ostream& errorlog) const=0;

	/// \brief Create a dump of the storage
	/// \return the object to fetch the dump from
	virtual StorageDumpInterface* createDump() const=0;
};

}//namespace
#endif


