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
#include "storageTransaction.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/statisticsBuilderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "storageDocument.hpp"
#include "storageClient.hpp"
#include "databaseAdapter.hpp"
#include "strus/arithmeticVariant.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace strus;

StorageTransaction::StorageTransaction(
		StorageClient* storage_,
		DatabaseClientInterface* database_,
		const MetaDataDescription* metadescr_,
		const conotrie::CompactNodeTrie* termnomap_,
		const Index& maxtypeno_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_metadescr(metadescr_)
	,m_attributeMap(database_)
	,m_metaDataMap(database_,metadescr_)
	,m_invertedIndexMap(database_)
	,m_forwardIndexMap(database_,maxtypeno_)
	,m_userAclMap(database_)
	,m_termTypeMap(database_,DatabaseKey::TermTypePrefix, storage_->createTypenoAllocator())
	,m_termValueMap(database_,DatabaseKey::TermValuePrefix, storage_->createTermnoAllocator(),termnomap_)
	,m_docIdMap(database_,DatabaseKey::DocIdPrefix, storage_->createDocnoAllocator())
	,m_userIdMap(database_,DatabaseKey::UserNamePrefix, storage_->createUsernoAllocator())
	,m_attributeNameMap(database_,DatabaseKey::AttributeKeyPrefix, storage_->createAttribnoAllocator())
	,m_nof_documents(0)
	,m_commit(false)
	,m_rollback(false)
	,m_errorhnd(errorhnd_)
{
	if (m_storage->getStatisticsProcessor() != 0)
	{
		m_termTypeMap.defineInv( &m_termTypeMapInv);
		m_termValueMap.defineInv( &m_termValueMapInv);
	}
}

StorageTransaction::~StorageTransaction()
{
	if (!m_rollback && !m_commit) rollback();
}

Index StorageTransaction::lookUpTermValue( const std::string& name)
{
	return m_termValueMap.lookUp( name);
}

Index StorageTransaction::getOrCreateTermValue( const std::string& name)
{
	bool isNew;
	return m_termValueMap.getOrCreate( name, isNew);
}

Index StorageTransaction::getOrCreateTermType( const std::string& name)
{
	bool isNew;
	return m_termTypeMap.getOrCreate( utils::tolower( name), isNew);
}

Index StorageTransaction::getOrCreateDocno( const std::string& name, bool& isNew)
{
	return m_docIdMap.getOrCreate( name, isNew);
}

Index StorageTransaction::getOrCreateUserno( const std::string& name, bool& isNew)
{
	return m_userIdMap.getOrCreate( name, isNew);
}

Index StorageTransaction::getOrCreateAttributeName( const std::string& name)
{
	bool isNew;
	return m_attributeNameMap.getOrCreate( utils::tolower( name), isNew);
}

void StorageTransaction::defineMetaData( const Index& docno, const std::string& varname, const ArithmeticVariant& value)
{
	m_metaDataMap.defineMetaData( docno, varname, value);
}

void StorageTransaction::deleteMetaData( const Index& docno, const std::string& varname)
{
	m_metaDataMap.deleteMetaData( docno, varname);
}

void StorageTransaction::deleteMetaData( const Index& docno)
{
	m_metaDataMap.deleteMetaData( docno);
}

void StorageTransaction::defineAttribute( const Index& docno, const std::string& varname, const std::string& value)
{
	Index varno = getOrCreateAttributeName( varname);
	m_attributeMap.defineAttribute( docno, varno, value);
}

void StorageTransaction::deleteAttribute( const Index& docno, const std::string& varname)
{
	Index varno = getOrCreateAttributeName( varname);
	m_attributeMap.deleteAttribute( docno, varno);
}

void StorageTransaction::deleteAttributes( const Index& docno)
{
	m_attributeMap.deleteAttributes( docno);
}

void StorageTransaction::defineAcl( const Index& userno, const Index& docno)
{
	m_userAclMap.defineUserAccess( userno, docno);
}

void StorageTransaction::deleteAcl( const Index& userno, const Index& docno)
{
	m_userAclMap.deleteUserAccess( userno, docno);
}

void StorageTransaction::deleteAcl( const Index& docno)
{
	m_userAclMap.deleteDocumentAccess( docno);
}

void StorageTransaction::definePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, const std::vector<Index>& posinfo)
{
	m_invertedIndexMap.definePosinfoPosting(
		termtype, termvalue, docno, posinfo);
}

void StorageTransaction::openForwardIndexDocument( const Index& docno)
{
	m_forwardIndexMap.openForwardIndexDocument( docno);
}

void StorageTransaction::defineForwardIndexTerm(
	const Index& typeno,
	const Index& pos,
	const std::string& termstring)
{
	m_forwardIndexMap.defineForwardIndexTerm( typeno, pos, termstring);
}

void StorageTransaction::closeForwardIndexDocument()
{
	m_forwardIndexMap.closeForwardIndexDocument();
}

void StorageTransaction::countDocument()
{
	m_nof_documents += 1;
}

void StorageTransaction::deleteIndex( const Index& docno)
{
	m_invertedIndexMap.deleteIndex( docno);
	m_forwardIndexMap.deleteIndex( docno);
}

void StorageTransaction::deleteUserAccessRights(
	const std::string& username)
{
	try
	{
		Index userno = m_userIdMap.lookUp( username);
		if (userno != 0)
		{
			m_userAclMap.deleteUserAccess( userno);
		}
	}
	CATCH_ERROR_MAP( _TXT("error deleting document user access rights in transaction: %s"), *m_errorhnd);
}

void StorageTransaction::deleteDocument( const std::string& docid)
{
	try
	{
		Index docno = m_docIdMap.lookUp( docid);
		if (docno == 0) return;

		//[1] Delete metadata:
		deleteMetaData( docno);

		//[2] Delete attributes:
		deleteAttributes( docno);

		//[3] Delete index elements (forward index and inverted index):
		deleteIndex( docno);

		//[4] Delete ACL elements:
		deleteAcl( docno);

		//[5] Delete the document id
		m_docIdMap.deleteKey( docid);
		m_nof_documents -= 1;
	}
	CATCH_ERROR_MAP( _TXT("error deleting document in transaction: %s"), *m_errorhnd);
}

StorageDocumentInterface*
	StorageTransaction::createDocument(
		const std::string& docid)
{
	try
	{
		bool isNew;
		Index dn = m_docIdMap.getOrCreate( docid, isNew);
		return new StorageDocument( this, docid, dn, isNew, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document in transaction: %s"), *m_errorhnd, 0);
}

StorageDocumentUpdateInterface*
	StorageTransaction::createDocumentUpdate(
		const Index& docno_)
{
	try
	{
		return new StorageDocumentUpdate( this, docno_, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating document update in transaction: %s"), *m_errorhnd, 0);
}

void StorageTransaction::updateMetaData(
		const Index& docno, const std::string& varname, const ArithmeticVariant& value)
{
	try
	{
		defineMetaData( docno, varname, value);
	}
	CATCH_ERROR_MAP( _TXT("error updating document meta data in transaction: %s"), *m_errorhnd);
}

bool StorageTransaction::commit()
{
	// Structure to make the rollback method be called in case of an exeption
	struct StatisticsBuilderScope
	{
		StatisticsBuilderScope( StatisticsBuilderInterface* obj_)
			:m_obj(obj_)
		{
			if (m_obj) m_obj->start();
		}
		void done()
		{
			m_obj = 0;
		}
		~StatisticsBuilderScope()
		{
			if (m_obj) m_obj->rollback();
		}

	private:
		 StatisticsBuilderInterface* m_obj;
	};
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called transaction commit twice"));
		return false;
	}
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called transaction commit after rollback"));
		return false;
	}
	if (m_errorhnd->hasError())
	{
		m_errorhnd->explain( _TXT( "storage transaction with error: %s"));
		return false;
	}
	try
	{
		StorageClient::TransactionLock lock( m_storage);
		//... we need a lock because transactions need to be sequentialized

		StatisticsBuilderInterface* statisticsBuilder = m_storage->getStatisticsBuilder();
		DocumentFrequencyCache* dfcache = m_storage->getDocumentFrequencyCache();

		std::auto_ptr<DatabaseTransactionInterface> transaction( m_database->createTransaction());
		if (!transaction.get())
		{
			m_errorhnd->explain( _TXT( "error creating transaction"));
			return false;
		}
		std::map<Index,Index> termnoUnknownMap;
		m_termValueMap.getWriteBatch( termnoUnknownMap, transaction.get());
		std::map<Index,Index> docnoUnknownMap;
		m_docIdMap.getWriteBatch( docnoUnknownMap, transaction.get());

		std::vector<Index> refreshList;
		m_attributeMap.renameNewDocNumbers( docnoUnknownMap);
		m_attributeMap.getWriteBatch( transaction.get());
		m_metaDataMap.renameNewDocNumbers( docnoUnknownMap);
		m_metaDataMap.getWriteBatch( transaction.get(), refreshList);

		m_invertedIndexMap.renameNewNumbers( docnoUnknownMap, termnoUnknownMap);

		StatisticsBuilderScope statisticsBuilderScope( statisticsBuilder);
		DocumentFrequencyCache::Batch dfbatch;

		m_invertedIndexMap.getWriteBatch(
				transaction.get(),
				statisticsBuilder, dfcache?&dfbatch:(DocumentFrequencyCache::Batch*)0,
				m_termTypeMapInv, m_termValueMapInv);
		if (statisticsBuilder)
		{
			statisticsBuilder->setNofDocumentsInsertedChange( m_nof_documents);
		}
		m_forwardIndexMap.renameNewDocNumbers( docnoUnknownMap);
		m_forwardIndexMap.getWriteBatch( transaction.get());

		m_userAclMap.renameNewDocNumbers( docnoUnknownMap);
		m_userAclMap.getWriteBatch( transaction.get());

		m_storage->getVariablesWriteBatch( transaction.get(), m_nof_documents);
		if (!transaction->commit())
		{
			m_errorhnd->explain(_TXT("error in database transaction commit: %s"));
			return false;
		}
		if (dfcache)
		{
			dfcache->writeBatch( dfbatch);
		}
		m_storage->declareNofDocumentsInserted( m_nof_documents);
		m_storage->releaseTransaction( refreshList);
		statisticsBuilderScope.done();

		m_commit = true;
		m_nof_documents = 0;
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in transaction commit: %s"), *m_errorhnd, false);
}

void StorageTransaction::rollback()
{
	if (m_rollback)
	{
		m_errorhnd->report( _TXT( "called transaction rollback twice"));
		return;
	}
	if (m_commit)
	{
		m_errorhnd->report( _TXT( "called transaction rollback after commit"));
		return;
	}
	std::vector<Index> refreshList;
	m_storage->releaseTransaction( refreshList);
	m_rollback = true;
}


