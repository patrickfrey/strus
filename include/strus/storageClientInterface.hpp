/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Client interface for accessing a storage of a search index (read/write)
/// \file "storageClientInterface.hpp"
#ifndef _STRUS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_CLIENT_INTERFACE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/timeStamp.hpp"
#include "strus/storage/statisticsMessage.hpp"
#include "strus/storage/blockStatistics.hpp"
#include "strus/storage/termStatistics.hpp"
#include <string>
#include <vector>
#include <ostream>

namespace strus
{

/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class StructureIteratorInterface;
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
class StorageDumpInterface;
/// \brief Forward declaration
class StatisticsProcessorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class MetaDataRestrictionInterface;
/// \brief Forward declaration
class AttributeReaderInterface;
/// \brief Forward declaration
class AclReaderInterface;


/// \brief Interface of a search index storage
class StorageClientInterface
{
public:
	/// \brief Destructor
	/// \remark Should call 'close()' but ignore errors returned silently
	virtual ~StorageClientInterface(){}

	/// \brief Reload storage client with altered configuration
	/// \param[in] config configuration string
	virtual bool reload( const std::string& config)=0;

	/// \brief Get the disk usage in kilo byte units (approximately) of the storage
	virtual long diskUsage() const=0;

	/// \brief Get the block type usage statistics
	virtual BlockStatistics blockStatistics() const=0;

	/// \brief Get the interpreted configuration this storage client was created with
	/// \return the configuration as string
	virtual std::string config() const=0;

	/// \brief Create an iterator on the occurrencies of a term in the storage
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \param[in] length ordinal position length assigned to the term (may differ from 1 for terms representing multipart patterns)
	/// \param[in] stats global term statistics
	/// \remark the length is considered as an attribute and not used in set operations for joining posting sets. It is used as hint only in some summarization and weighting functions for handling multi-word phrases correctly.
	/// \return the created iterator reference (with ownership)
	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value,
			const Index& length,
			const TermStatistics& stats) const=0;

	/// \brief Create an iterator on the document term occurrence frequencies in the storage. In opposite to the term posting iterator its skip position method returns only position = 1 for any matching document
	/// \note This posting iterator is used when disabling position information as criterion in the query. This may be suitable when using simple query evaluation methods that are just considering the number of occurrencies and not taking positions (e.g. for proximity) into account.
	/// \remark Be aware that switching of positions may lead to different results even when using query evaluation schemes that do not take positions into account. This is the case if your features are complex expression using positions in their joins. Therefore positions have to be explicitely switched of in the query and are not enabled or disabled by discovery.
	/// \note In strus the use of frequeny posting iterators is encouraged in a form of initial query evaluation that decides what documents to select for feature extraction for query expansion.
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \param[in] stats global term statistics
	/// \return the created iterator reference (with ownership)
	virtual PostingIteratorInterface*
		createFrequencyPostingIterator(
			const std::string& type,
			const std::string& value,
			const TermStatistics& stats) const=0;

	/// \brief Create an iterator on the structures (relations of ordinal position ranges with a structure name)
	/// \return the created iterator reference (with ownership)
	virtual StructureIteratorInterface*
		createStructureIterator() const=0;

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
	/// \note The storage has to be created with access control enabled
	virtual InvAclIteratorInterface*
		createInvAclIterator(
			const std::string& username) const=0;

	/// \brief Create a an iterator on the access control lists of documents
	/// \return the iterator on the ACLs
	/// \note The storage has to be created with access control enabled
	virtual AclReaderInterface* createAclReader() const=0;

	/// \brief Get the number of documents inserted in this storage instance
	/// \return the number of documents
	virtual Index nofDocumentsInserted() const=0;

	/// \brief Get the local document frequency of a feature in this storage instance
	/// \param[in] type the term type addressed
	/// \param[in] term the term value addressed
	/// \return the local document frequency
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

	/// \brief Get the number assigned to a structure name
	/// \return the structure number >= 1 or 0 if undefined
	virtual Index structTypeNumber( const std::string& structname) const=0;

	/// \brief Get the local internal term type number
	/// \param[in] type term type name
	/// \return the term type number or 0, if it is not known yet
	virtual Index termTypeNumber( const std::string& type) const=0;

	/// \brief Get the local internal term value number
	/// \param[in] value term value
	/// \return the term value number or 0, if it is not known yet
	virtual Index termValueNumber( const std::string& type) const=0;

	/// \brief Evaluate if there exists forward index blocks for this type
	/// \param[in] type term type name
	/// \return true, if yes, false if no or if an error occurred (check error)
	virtual bool isForwardIndexTerm( const std::string& type) const=0;

	/// \brief Create an iterator on the term types inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createTermTypeIterator() const=0;

	/// \brief Create an iterator on the structure types inserted
	/// \return the iterator
	virtual ValueIteratorInterface* createStructTypeIterator() const=0;

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

	/// \brief Get the next change statistics timestamp that is bigger than the one specified as argument
	/// \note Returns -1 with error if no statistics processor instance defined for this storage client
	/// \note Depending on the higher level configuration of the system, older files with statistic changes are deleted and not accessible anymore
	/// \param[in] timestamp time of last change statistics fetched
	/// \return the next timestamp
	virtual TimeStamp getNextChangeStatisticsTimeStamp( const TimeStamp& timestamp) const=0;

	/// \brief Load the one incremental statistics change message associated with a timestamp
	/// \note Returns an empty message with error if no statistics processor instance defined for this storage client
	/// \param[in] timestamp timestamp associated with the statistics change message
	/// \return the statistics change message structure
	virtual StatisticsMessage loadChangeStatisticsMessage( const TimeStamp& timestamp) const=0;

	/// \brief Load all statistics
	/// \return the list of all statistics
	virtual std::vector<StatisticsMessage> loadAllStatisticsMessages() const=0;

	/// \brief Get the processing message interface for introspecting and packing messages outside the queue context
	/// \return the message processor interface
	virtual const StatisticsProcessorInterface* getStatisticsProcessor() const=0;

	/// \brief Create an interface to verify, if the contents of a document are inserted correctly into the storage. The checking is invoked by calling the StorageDocumentInterface::done() method after the definition of all elements.
	/// \param[in] docid identifier (URI) of the document to check
	/// \param[in] logfilename Where to log checking failures ("-" for stdout, "" for the error buffer interface)
	/// \return the created document interface (with ownership)
	virtual StorageDocumentInterface* createDocumentChecker(
			const std::string& docid,
			const std::string& logfilename = std::string()) const=0;

	/// \brief Create a dump of a storage to iterate on
	/// \param[in] keyprefix prefix for keys to resrict the dump to
	/// \return the object to fetch the dump from
	virtual StorageDumpInterface* createDump(
			const std::string& keyprefix) const=0;

	/// \brief Get a list of configuration parameters that can be used in reload (e.g. same as for createClient)
	/// \return NULL terminated array of config parameters
	virtual const char** getConfigParameters() const=0;

	/// \brief Iterate through all key/value pairs and check their data for validity
	/// \param[out] errorlog stream for reporting errors
	/// \return true, if the check succeeds, false if it fails
	virtual bool checkStorage( std::ostream& errorlog) const=0;

	/// \brief Close client connection and eventually do some cleanup.
	/// \remark This method is not implicitely called with the destructor because it might be a complicated operation that cannot be afforded in panic shutdown.
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void close()=0;

	/// \brief Do compaction of data.
	/// \remark This method is also called as side effect close
	/// \note the method does not have to be called necessarily
	/// \note it calls compactDatabase of the underlying database and can therefore last some time (some minutes in case of leveldb after large inserts).
	virtual void compaction()=0;
};

}//namespace
#endif


