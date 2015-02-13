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
#include "peerStorageTransaction.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "storage.hpp"
#include <boost/thread/mutex.hpp>

using namespace strus;

PeerStorageTransaction::PeerStorageTransaction( Storage* storage_, DatabaseInterface* database_, DocumentFrequencyCache* dfcache_)
	:m_storage(storage_)
	,m_database(database_)
	,m_documentFrequencyCache(dfcache_)
	,m_dbadapter_termvalue(database_)
	,m_termvaluecnt(0)
	,m_nofDocumentsInserted(0)
	,m_commit(false)
	,m_rollback(false)
{}

PeerStorageTransaction::~PeerStorageTransaction()
{
	if (!m_rollback && !m_commit) rollback();
}

void PeerStorageTransaction::updateNofDocumentsInsertedChange( int increment)
{
	m_nofDocumentsInserted += increment;
}

void PeerStorageTransaction::updateDocumentFrequencyChange(
		const char* termtype, const char* termvalue, int increment)
{
	bool typeno_isNew = false;
	Index typeno = m_storage->allocTypenoIm( termtype, typeno_isNew);
	Index termno = m_dbadapter_termvalue.get( termvalue);
	if (!termno)
	{
		m_unknownTerms.push_back( termvalue);
		termno = ++m_termvaluecnt + UnknownValueHandleStart;
		if (m_termvaluecnt >= UnknownValueHandleStart)
		{
			throw std::runtime_error("too many new terms inserted (peer storage transaction");
		}
	}
	m_dfbatch.put( typeno, termno, increment);
}

void PeerStorageTransaction::commit()
{
	if (m_commit)
	{
		throw std::runtime_error( "called transaction commit twice");
	}
	if (m_rollback)
	{
		throw std::runtime_error( "called transaction commit after rollback");
	}
	Reference<DatabaseTransactionInterface> transaction( m_database->createTransaction());

	Storage::TransactionLock lock( m_storage);
	DocumentFrequencyCache::Batch cleaned_batch;
	DocumentFrequencyCache::Batch::const_iterator bi = m_dfbatch.begin(), be = m_dfbatch.end();
	for (; bi != be; ++bi)
	{
		if (bi->termno > UnknownValueHandleStart)
		{
			const std::string& termkey = m_unknownTerms[ bi->termno - UnknownValueHandleStart - 1];
			Index termno = m_dbadapter_termvalue.get( termkey);
			if (!termno)
			{
				termno = m_storage->allocTermno();
				m_dbadapter_termvalue.store( transaction.get(), termkey, termno);
				cleaned_batch.put( bi->typeno, termno, bi->value);
			}
		}
		else
		{
			cleaned_batch.put( *bi);
		}
	}
	m_documentFrequencyCache->writeBatch( cleaned_batch);
	transaction->commit();
	m_storage->declareGlobalNofDocumentsInserted( m_nofDocumentsInserted);
	m_commit = true;
	m_nofDocumentsInserted = 0;
}

void PeerStorageTransaction::rollback()
{
	if (m_commit)
	{
		throw std::runtime_error( "called transaction rollback twice");
	}
	if (m_rollback)
	{
		throw std::runtime_error( "called transaction rollback after commit");
	}
	m_rollback = true;
}

