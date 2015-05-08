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
#include "storageTransaction.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/storageDocumentInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storagePeerInterface.hpp"
#include "strus/storagePeerTransactionInterface.hpp"
#include "storageDocument.hpp"
#include "storageClient.hpp"
#include "databaseAdapter.hpp"
#include "strus/arithmeticVariant.hpp"
#include "private/internationalization.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace strus;

StorageTransaction::StorageTransaction(
		StorageClient* storage_,
		DatabaseClientInterface* database_,
		const StoragePeerInterface* storagePeer_,
		const MetaDataDescription* metadescr_,
		const conotrie::CompactNodeTrie* termnomap_)
	:m_storage(storage_)
	,m_database(database_)
	,m_storagePeer(storagePeer_)
	,m_metadescr(metadescr_)
	,m_attributeMap(database_)
	,m_metaDataMap(database_,metadescr_)
	,m_invertedIndexMap(database_)
	,m_forwardIndexMap(database_)
	,m_userAclMap(database_)
	,m_termTypeMap(database_,DatabaseKey::TermTypePrefix, storage_->createTypenoAllocator())
	,m_termValueMap(database_,DatabaseKey::TermValuePrefix, storage_->createTermnoAllocator(),termnomap_)
	,m_docIdMap(database_,DatabaseKey::DocIdPrefix, storage_->createDocnoAllocator())
	,m_userIdMap(database_,DatabaseKey::UserNamePrefix, storage_->createUsernoAllocator())
	,m_attributeNameMap(database_,DatabaseKey::AttributeKeyPrefix, storage_->createAttribnoAllocator())
	,m_nof_documents(0)
	,m_commit(false)
	,m_rollback(false)
{
	if (m_storagePeer)
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



void StorageTransaction::deleteIndex( const Index& docno)
{
	m_invertedIndexMap.deleteIndex( docno);
	m_forwardIndexMap.deleteIndex( docno);
}

void StorageTransaction::deleteUserAccessRights(
	const std::string& username)
{
	Index userno = m_userIdMap.lookUp( username);
	if (userno != 0)
	{
		m_userAclMap.deleteUserAccess( userno);
	}
}

void StorageTransaction::deleteDocument( const std::string& docid)
{
	Index docno = m_docIdMap.lookUp( docid);
	if (docno == 0) return;

	//[1] Delete metadata:
	deleteMetaData( docno);

	//[2] Delete attributes:
	deleteAttributes( docno);
	
	//[3] Delete index elements (forward index and inverted index):
	deleteIndex( docno);

	//[3] Delete ACL elements:
	deleteAcl( docno);

	m_nof_documents -= 1;
}

StorageDocumentInterface*
	StorageTransaction::createDocument(
		const std::string& docid,
		const Index& docno)
{
	if (docno)
	{
		m_nof_documents += 1;
		m_newDocidMap[ docid] = docno;
		return new StorageDocument( this, docid, docno, true);
	}
	else
	{
		bool isNew;
		Index dn = m_docIdMap.getOrCreate( docid, isNew);
		if (isNew) m_nof_documents += 1;
		return new StorageDocument( this, docid, dn, isNew);
	}
}

StorageDocumentUpdateInterface*
	StorageTransaction::createDocumentUpdate(
		const Index& docno_)
{
	return new StorageDocumentUpdate( this, docno_);
}

void StorageTransaction::updateMetaData(
		const Index& docno, const std::string& varname, const ArithmeticVariant& value)
{
	defineMetaData( docno, varname, value);
}

void StorageTransaction::commit()
{
	if (m_commit)
	{
		throw strus::runtime_error( _TXT( "called transaction commit twice"));
	}
	if (m_rollback)
	{
		throw strus::runtime_error( _TXT( "called transaction commit after rollback"));
	}
	{
		StorageClient::TransactionLock lock( m_storage);
		//... we need a lock because transactions need to be sequentialized

		std::auto_ptr<DatabaseTransactionInterface> transaction( m_database->createTransaction());

		std::map<Index,Index> termnoUnknownMap;
		m_termValueMap.getWriteBatch( termnoUnknownMap, transaction.get());

		std::vector<Index> refreshList;
		m_attributeMap.getWriteBatch( transaction.get());
		m_metaDataMap.getWriteBatch( transaction.get(), refreshList);
	
		m_invertedIndexMap.renameNewTermNumbers( termnoUnknownMap);

		Reference<StoragePeerTransactionInterface> peerTransaction;
		if (m_storagePeer)
		{
			peerTransaction.reset( m_storagePeer->createTransaction());
		}
		m_invertedIndexMap.getWriteBatch(
				transaction.get(),
				peerTransaction.get(),
				m_termTypeMapInv, m_termValueMapInv);

		m_forwardIndexMap.getWriteBatch( transaction.get());
	
		m_userAclMap.getWriteBatch( transaction.get());
	
		std::map<std::string,Index>::const_iterator di = m_newDocidMap.begin(), de = m_newDocidMap.end();
		for (; di != de; ++di)
		{
			DatabaseAdapter_DocId::Writer( m_database).store( transaction.get(), di->first, di->second);
		}

		m_storage->getVariablesWriteBatch( transaction.get(), m_nof_documents);
		if (m_storagePeer)
		{
			peerTransaction->populateNofDocumentsInsertedChange( m_nof_documents);
			peerTransaction->try_commit();
			transaction->commit();
			peerTransaction->final_commit();
		}
		else
		{
			transaction->commit();
		}
		m_storage->declareNofDocumentsInserted( m_nof_documents);
		m_storage->releaseTransaction( refreshList);
	}
	m_commit = true;
	m_nof_documents = 0;
}

void StorageTransaction::rollback()
{
	if (m_rollback)
	{
		throw strus::runtime_error( _TXT( "called transaction rollback twice"));
	}
	if (m_commit)
	{
		throw strus::runtime_error( _TXT( "called transaction rollback after commit"));
	}
	std::vector<Index> refreshList;
	m_storage->releaseTransaction( refreshList);
	m_rollback = true;
}

