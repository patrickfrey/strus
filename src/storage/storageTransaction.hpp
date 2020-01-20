/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_TRANSACTION_HPP_INCLUDED
#define _STRUS_STORAGE_TRANSACTION_HPP_INCLUDED
#include "strus/storageTransactionInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/reference.hpp"
#include "metaDataBlock.hpp"
#include "metaDataRecord.hpp"
#include "metaDataMap.hpp"
#include "metaDataReader.hpp"
#include "storageMetaDataTransaction.hpp"
#include "attributeMap.hpp"
#include "booleanBlock.hpp"
#include "userAclMap.hpp"
#include "posinfoBlock.hpp"
#include "invertedIndexMap.hpp"
#include "structIndexMap.hpp"
#include "forwardIndexBlock.hpp"
#include "forwardIndexMap.hpp"
#include "keyMap.hpp"
#include "keyMapInv.hpp"
#include "keyAllocatorInterface.hpp"
#include "private/stringMap.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

namespace strus {

/// \brief Forward declaration
class StorageClient;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageMetaDataTableUpdateInterface;


/// \class StorageTransaction
class StorageTransaction
	:public StorageTransactionInterface
{
public:
	///\param[in] storage_ storage to call refresh after commit or rollback
	///\param[in] maxtypeno_ maximum type number
	///\param[in] errorhnd_ error buffer interface
	StorageTransaction( 
		StorageClient* storage_,
		strus::Index maxtypeno_,
		ErrorBufferInterface* errorhnd_);

	~StorageTransaction();

	virtual void deleteDocument(
			const std::string& docid);

	virtual void deleteUserAccessRights(
			const std::string& username);

	virtual StorageDocumentInterface*
		createDocument(
			const std::string& docid_);

	virtual StorageDocumentUpdateInterface*
		createDocumentUpdate(
			const Index& docno_);

	virtual void updateMetaData(
			const Index& docno, const std::string& varname, const NumericVariant& value);

	virtual void updateDocumentFrequency(
			const std::string& type, const std::string& value, int df_change);

	virtual StorageMetaDataTableUpdateInterface* createMetaDataTableUpdate();

	virtual StorageCommitResult commit();
	virtual void rollback();

public:/*Document,DocumentUpdate*/
	Index getOrCreateTermValue( const std::string& name);
	Index getOrCreateTermType( const std::string& name);
	Index getOrCreateStructType( const std::string& name);
	Index getOrCreateAttributeName( const std::string& name);
	Index getOrCreateDocno( const std::string& name);
	Index getOrCreateUserno( const std::string& name);
	Index lookUpTermValue( const std::string& name);

	void defineMetaData( strus::Index docno, const std::string& varname, const NumericVariant& value);
	void deleteMetaData( strus::Index docno, const std::string& varname);
	void deleteMetaData( strus::Index docno);

	void defineAttribute( strus::Index docno, const std::string& varname, const std::string& value);
	void deleteAttribute( strus::Index docno, const std::string& varname);
	void deleteAttributes( strus::Index docno);

	void defineAcl( strus::Index userno, strus::Index docno);
	void deleteAcl( strus::Index userno, strus::Index docno);
	void deleteAcl( strus::Index docno);

	void deleteIndex( strus::Index docno);
	void deleteDocSearchIndexType( strus::Index docno, strus::Index typeno);
	void deleteDocForwardIndexType( strus::Index docno, strus::Index typeno);

	void definePosinfoPosting(
		strus::Index termtype, strus::Index termvalue,
		strus::Index docno, const std::vector<Index>& posinfo);

	void deleteStructures( strus::Index docno);
	void defineStructureBlock( strus::Index docno, const StructBlock& blk);

	void openForwardIndexDocument( strus::Index docno);

	void defineForwardIndexTerm(
		strus::Index typeno, strus::Index pos, const std::string& termstring);

	void closeForwardIndexDocument();

private:
	void reset();
	StorageCommitResult commit_contentTransaction();

private:
	StorageClient* m_storage;				///< storage to call refresh after commit or rollback
	strus::Reference<StorageMetaDataTransaction> m_metadataTransaction;	///< transaction for altering meta data table structure

	AttributeMap m_attributeMap;				///< map of document attributes for writing
	MetaDataMap m_metaDataMap;				///< map of meta data blocks for writing

	InvertedIndexMap m_invertedIndexMap;			///< map of posinfo postings for writing
	StructIndexMap m_structIndexMap;			///< map of structures for writing
	ForwardIndexMap m_forwardIndexMap;			///< map of forward index for writing
	UserAclMap m_userAclMap;				///< map of user rights for writing (forward and inverted)

	KeyMap m_termTypeMap;					///< map of term types
	KeyMap m_structTypeMap;					///< map of struct types
	KeyMap m_termValueMap;					///< map of term values
	KeyMap m_docIdMap;					///< map of document ids
	KeyMap m_userIdMap;					///< map of user ids
	KeyMap m_attributeNameMap;				///< map of document attribute names

	KeyMapInv m_termTypeMapInv;				///< inverse map of term types
	KeyMapInv m_termValueMapInv;				///< inverse map of term values

	DocumentFrequencyMap m_explicit_dfmap;			///< df map for features not in search index with explicit df change

	int m_nofDeletedDocuments;				///< total adjustment for the number of documents deleted
	int m_nofOperations;					///< number of atering operations in this transaction without counting meta data table structure operations, used to decide wheter this transaction in changing meta data or content */

	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif

