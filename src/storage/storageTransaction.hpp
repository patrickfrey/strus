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


/// \class StorageTransaction
class StorageTransaction
	:public StorageTransactionInterface
{
public:
	///\param[in] storage_ storage to call refresh after commit or rollback
	///\param[in] database_ database handle
	///\param[in] maxtypeno_ biggest type number to use in this transaction in the forward index map when deleting elements there
	StorageTransaction( 
		StorageClient* storage_,
		const Index& maxtypeno_,
		const Index& maxstructno_,
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

	virtual bool commit();
	virtual void rollback();

	virtual unsigned int nofDocumentsAffected() const
	{
		return m_nof_documents_affected;
	}

public:/*Document,DocumentUpdate*/
	Index getOrCreateTermValue( const std::string& name);
	Index getOrCreateTermType( const std::string& name);
	Index getOrCreateStructType( const std::string& name);
	Index getOrCreateAttributeName( const std::string& name);
	Index getOrCreateDocno( const std::string& name);
	Index getOrCreateUserno( const std::string& name);
	Index lookUpTermValue( const std::string& name);

	void defineMetaData( const Index& docno, const std::string& varname, const NumericVariant& value);
	void deleteMetaData( const Index& docno, const std::string& varname);
	void deleteMetaData( const Index& docno);

	void defineAttribute( const Index& docno, const std::string& varname, const std::string& value);
	void deleteAttribute( const Index& docno, const std::string& varname);
	void deleteAttributes( const Index& docno);

	void defineAcl( const Index& userno, const Index& docno);
	void deleteAcl( const Index& userno, const Index& docno);
	void deleteAcl( const Index& docno);

	void deleteIndex( const Index& docno);
	void deleteDocSearchIndexType( const Index& docno, const Index& typeno);
	void deleteDocSearchIndexStructure( const Index& docno, const Index& structno);
	void deleteDocForwardIndexType( const Index& docno, const Index& typeno);

	void definePosinfoPosting(
		const Index& termtype, const Index& termvalue,
		const Index& docno, const std::vector<Index>& posinfo);

	void defineStructure(
		const Index& structno,
		const Index& docno, const IndexRange& source, const IndexRange& sink);

	void openForwardIndexDocument( const Index& docno);

	void defineForwardIndexTerm(
		const Index& typeno, const Index& pos, const std::string& termstring);

	void closeForwardIndexDocument();

private:
	void clearMaps();

private:
	StorageClient* m_storage;				///< storage to call refresh after commit or rollback

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

	int m_nof_deleted_documents;				///< total adjustment for the number of documents deleted
	int m_nof_documents_affected;				///< total number of documents affected by last transaction

	bool m_commit;						///< true, if the transaction has been committed
	bool m_rollback;					///< true, if the transaction has been rolled back

	ErrorBufferInterface* m_errorhnd;			///< error buffer for exception free interface
};

}//namespace
#endif

