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
#include "storageDocument.hpp"
#include "storage.hpp"
#include "strus/arithmeticVariant.hpp"
#include <vector>
#include <string>
#include <set>
#include <map>

using namespace strus;

StorageTransaction::StorageTransaction(
		Storage* storage_, leveldb::DB* db_,
		const MetaDataDescription* metadescr_)
	:m_storage(storage_)
	,m_db(db_)
	,m_metadescr(metadescr_)
	,m_dfMap(db_)
	,m_attributeMap(db_)
	,m_metaDataBlockMap(db_,metadescr_)
	,m_docnoBlockMap(db_)
	,m_posinfoBlockMap(db_)
	,m_forwardIndexBlockMap(db_)
	,m_termTypeMap(db_,DatabaseKey::TermTypePrefix, storage_->createTypenoAllocator())
	,m_termValueMap(db_,DatabaseKey::TermValuePrefix, storage_->createTermnoAllocator())
	,m_docIdMap(db_,DatabaseKey::DocIdPrefix, storage_->createDocnoAllocator())
	,m_attributeNameMap(db_,DatabaseKey::AttributeKeyPrefix, storage_->createAttribnoAllocator())
	,m_nof_documents(0)
	,m_commit(false)
	,m_rollback(false)
{}

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
	return m_termTypeMap.getOrCreate( name, isNew);
}

Index StorageTransaction::getOrCreateDocno( const std::string& name, bool& isNew)
{
	return m_docIdMap.getOrCreate( name, isNew);
}

Index StorageTransaction::getOrCreateAttributeName( const std::string& name)
{
	bool isNew;
	return m_attributeNameMap.getOrCreate( name, isNew);
}

void StorageTransaction::incrementDf( const Index& typeno, const Index& termno)
{
	m_dfMap.increment( typeno, termno);
}

void StorageTransaction::decrementDf( const Index& typeno, const Index& termno)
{
	m_dfMap.decrement( typeno, termno);
}

void StorageTransaction::defineMetaData( const Index& docno, const std::string& varname, const ArithmeticVariant& value)
{
	m_metaDataBlockMap.defineMetaData( docno, varname, value);
}

void StorageTransaction::deleteMetaData( const Index& docno, const std::string& varname)
{
	m_metaDataBlockMap.deleteMetaData( docno, varname);
}

void StorageTransaction::deleteMetaData( const Index& docno)
{
	m_metaDataBlockMap.deleteMetaData( docno);
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

void StorageTransaction::defineDocnoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, unsigned int ff, float weight)
{
	m_docnoBlockMap.defineDocnoPosting(
		termtype, termvalue, docno, ff, weight);
}

void StorageTransaction::deleteDocnoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno)
{
	m_docnoBlockMap.deleteDocnoPosting( termtype, termvalue, docno);
}

void StorageTransaction::definePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno, const std::vector<Index>& posinfo)
{
	m_posinfoBlockMap.definePosinfoPosting(
		termtype, termvalue, docno, posinfo);
}

void StorageTransaction::deletePosinfoPosting(
	const Index& termtype, const Index& termvalue,
	const Index& docno)
{
	m_posinfoBlockMap.deletePosinfoPosting( termtype, termvalue, docno);
}

void StorageTransaction::defineForwardIndexTerm(
	const Index& typeno,
	const Index& docno,
	const Index& pos,
	const std::string& termstring)
{
	m_forwardIndexBlockMap.defineForwardIndexTerm( typeno, docno, pos, termstring);
}

void StorageTransaction::deleteForwardIndexTerm(
	const Index& typeno,
	const Index& docno,
	const Index& pos)
{
	m_forwardIndexBlockMap.deleteForwardIndexTerm( typeno, docno, pos);
}

class TermnoMap
{
public:
	TermnoMap( StorageTransaction* transaction_)
		:m_transaction(transaction_){}

	Index operator()( const std::string& value)
	{
		return m_transaction->lookUpTermValue( value);
	}

private:
	StorageTransaction* m_transaction;
};

void StorageTransaction::deleteIndex( const Index& docno)
{
	typedef std::pair<Index,Index> TermMapKey;
	std::set<TermMapKey> oldcontent;

	KeyValueStorage fwstorage( m_db, DatabaseKey::ForwardIndexPrefix, false);
	TermnoMap termnoMap( this);

	Index ti = 1, te = m_storage->nofAttributeTypes();
	for (; ti <= te; ++ti)
	{
		std::map<Index,Index> countmap
			= m_forwardIndexBlockMap.getTermOccurrencies(
				ti, docno, termnoMap);
		std::map<Index,Index>::const_iterator
			ci = countmap.begin(), ce = countmap.end();
		for (; ci != ce; ++ci)
		{
			deleteDocnoPosting( ti, ci->first, docno);
			deletePosinfoPosting( ti, ci->first, docno);
	
			decrementDf( ti, ci->first);
		}
		fwstorage.disposeSubnodes( BlockKey( ti, docno), m_batch);
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

void StorageTransaction::commit()
{
	if (m_commit)
	{
		throw std::runtime_error( "called transaction commit twice");
	}
	if (m_rollback)
	{
		throw std::runtime_error( "called transaction commit after rollback");
	}
	Storage::TransactionLock transactionLock( m_storage);
	{
		std::map<Index,Index> termnoUnknownMap;
		m_termValueMap.getWriteBatch( termnoUnknownMap, m_batch);

		m_docnoBlockMap.renameNewTermNumbers( termnoUnknownMap);
		m_posinfoBlockMap.renameNewTermNumbers( termnoUnknownMap);

		std::vector<Index> refreshList;
		m_attributeMap.getWriteBatch( m_batch);
		m_metaDataBlockMap.getWriteBatch( m_batch, refreshList);

		m_docnoBlockMap.getWriteBatch( m_batch);
		m_posinfoBlockMap.getWriteBatch( m_batch);
		m_forwardIndexBlockMap.getWriteBatch( m_batch);

		m_dfMap.renameNewTermNumbers( termnoUnknownMap);
		m_dfMap.getWriteBatch( m_batch);

		KeyValueStorage docidstor( m_db, DatabaseKey::DocIdPrefix, false);
		std::map<std::string,Index>::const_iterator di = m_newDocidMap.begin(), de = m_newDocidMap.end();
		for (; di != de; ++di)
		{
			std::string docnostr;
			packIndex( docnostr, di->second);
			docidstor.store( di->first, docnostr, m_batch);
		}

		leveldb::WriteOptions options;
		options.sync = true;
		leveldb::Status status = m_db->Write( options, &m_batch);
		if (!status.ok())
		{
			throw std::runtime_error( std::string( "error in commit when writing transaction batch: ") + status.ToString());
		}
		m_storage->declareNofDocumentsInserted( m_nof_documents);
		m_storage->releaseTransaction( refreshList);
	}
	m_batch.Clear();
	m_commit = true;
	m_nof_documents = 0;
}

void StorageTransaction::rollback()
{
	if (m_rollback)
	{
		throw std::runtime_error( "called transaction rollback twice");
	}
	if (m_commit)
	{
		throw std::runtime_error( "called transaction rollback after commit");
	}
	std::vector<Index> refreshList;
	m_storage->releaseTransaction( refreshList);
	m_batch.Clear();
	m_rollback = true;
}

