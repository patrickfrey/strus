/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
class MetaDataReaderInterface;
/// \brief Forward declaration
class MetaDataRestrictionInterface;
/// \brief Forward declaration
class AttributeReaderInterface;


/// \brief Interface of a strus IR storage
class StorageClientInterface
{
public:
	/// \brief Destructor
	/// \remark Should call call 'close()' but ignore errors there silently
	virtual ~StorageClientInterface(){}

	/// \brief Get the interpreted configuration this storage client was created with
	/// \return the configuration as string
	virtual std::string config() const=0;

	/// \brief Create an iterator on the occurrencies of a term in the storage
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \param[in] length ordinal position length assigned to the term (may differ from 1 for terms representing multipart patterns)
	/// \remark the length is considered as an attribute and not used in set operations for joining posting sets. It is used as hint only in some summarization and weighting functions.
	/// \return the created iterator reference (with ownership)
	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value,
			const Index& length) const=0;

	/// \brief Create an iterator on all enumerable postings of document selected by a metadata restriction
	/// \param[in] restriction restriction on metadata that have to be fulfilled by the documents referenced in the result posting sets.
	/// \param[in] maxpos maximum position visited. 
	/// \return the created iterator reference (with ownership)
	/// \note This iterator use is mainly for browsing occurrencies fulfilling a condition without query involved
	/// \remark The iterator does not take the alive status of the documents into account. You have to formulate a restriction expression that does not match deleted documents if you do not want deleted documents in your iterated set of postings.
	/// \remark The iterator does not take the document length into account. It returns the set of postings with positions in the range [1..maxpos]. Read postings you get only when joining this set with another.
	virtual PostingIteratorInterface*
		createBrowsePostingIterator(
			const MetaDataRestrictionInterface* restriction,
			const Index& maxpos) const=0;

	/// \brief Create an iterator postings specified as field in the meta data (by start and end position)
	/// \param[in] meta_fieldStart meta data element that specifies the start of the field
	/// \param[in] meta_fieldEnd meta data element that specifies the end of the field (first position not belonging to the field anymore)
	/// \return the created iterator reference (with ownership)
	/// \note Fields can avoid superfluos access on posting blocks
	/// \note Fields give you the possibility to treat elements in the document differently while using the same statistics that are defined by feature type
	virtual PostingIteratorInterface*
		createFieldPostingIterator(
			const std::string& meta_fieldStart,
			const std::string& meta_fieldEnd) const=0;

	/// \brief Create a viewer to inspect the term stored values with the forward index of the storage
	/// \param[in] type type name of the term to be inspected
	/// \return the created viewer reference to be disposed with delete
	virtual ForwardIteratorInterface*
		createForwardIterator(
			const std::string& type) const=0;

	/// \brief Create an iterator on term occurrencies in documents (support for feature selection)
	/// \param[in] type type name of the term
	/// \return the created iterator reference (with ownership)
	virtual DocumentTermIteratorInterface*
		createDocumentTermIterator(
			const std::string& type) const=0;

	/// \brief Create a an iterator on the numbers of documents a specified user is allowed to see
	/// \param[in] username name of the user
	/// \return the iterator on the documents (with ownership) or NULL, if there is no access control enabled
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

	/// \brief Get one specified element of the documents statistics for a term type on the local server node
	/// \param[in] docno the local internal document number addressed (return value of documentNumber( const std::string&) const)
	/// \param[in] stat the enumeration value of the statistics to get
	/// \param[in] type the term type addressed
	virtual Index documentStatistics(
			const Index& docno,
			const DocumentStatisticsType& stat,
			const std::string& type) const=0;

	/// \brief Create an interface to access items of document metadata
	/// \return the interface to access document metadata (with ownership)
	virtual MetaDataReaderInterface* createMetaDataReader() const=0;

	/// \brief Create an object for restrictions on metadata
	/// \return the created, uninitialized restriction object
	virtual MetaDataRestrictionInterface* createMetaDataRestriction() const=0;

	/// \brief Create an interface to access attributes attached to documents for representation
	/// \return the interface to access document attributes (with ownership)
	virtual AttributeReaderInterface* createAttributeReader() const=0;

	/// \brief Create an insert/update transaction object
	/// \return the created transaction interface (with ownership)
	/// \note this function is thread safe, multiple concurrent transactions are allowed 
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
	/// \return the created document interface (with ownership)
	virtual StorageDocumentInterface* createDocumentChecker(
			const std::string& docid,
			const std::string& logfilename) const=0;

	/// \brief Iterate through all key/value pairs and check their data for validity
	/// \param[out] errorlog stream for reporting errors
	/// \return true, if the check succeeds, false if it fails
	virtual bool checkStorage( std::ostream& errorlog) const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	virtual void close()=0;
};

}//namespace
#endif


