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
#include "private/internationalization.hpp"
#include "storageClient.hpp"

using namespace strus;

PeerStorageTransaction::PeerStorageTransaction( StorageClient* storage_, DatabaseClientInterface* database_, DocumentFrequencyCache* dfcache_)
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

void PeerStorageTransaction::updateNofDocumentsInsertedChange( const GlobalCounter& increment)
{
	m_nofDocumentsInserted += increment;
}

void PeerStorageTransaction::updateDocumentFrequencyChange(
		const char* termtype, const char* termvalue, const GlobalCounter& increment, bool isNew)
{
	if (m_commit)
	{
		throw strus::runtime_error( _TXT( "called update document frequeny change after commit"));
	}
	bool typeno_isNew = false;
	Index typeno = m_storage->allocTypenoImm( termtype, typeno_isNew);
	Index termno = m_dbadapter_termvalue.get( termvalue);
	if (!termno)
	{
		m_unknownTerms.push_back( m_unknownTerms_strings.size());
		m_unknownTerms_strings.append( termvalue);
		m_unknownTerms_strings.push_back( '\0');
		termno = ++m_termvaluecnt + UnknownValueHandleStart;
		if (m_termvaluecnt >= UnknownValueHandleStart)
		{
			throw strus::runtime_error( _TXT( "too many new terms inserted (peer storage transaction"));
		}
		if (isNew)
		{
			if (m_typeStrings.size() <= (std::size_t)typeno)
			{
				m_typeStrings.resize( typeno);
			}
			if (m_typeStrings[ typeno].empty())
			{
				m_typeStrings[ typeno] = termtype;
			}
			m_newTerms.push_back( NewTerm( m_newTerms_strings.size(), m_dfbatch.size()));
			m_newTerms_strings.append( termvalue);
			m_newTerms_strings.push_back( '\0');
		}
	}
	m_dfbatch.put( typeno, termno, increment);
}

std::vector<PeerStorageTransactionInterface::DocumentFrequencyChange> PeerStorageTransaction::commit()
{
	if (m_commit)
	{
		throw strus::runtime_error( _TXT( "called transaction commit twice"));
	}
	if (m_rollback)
	{
		throw strus::runtime_error( _TXT( "called transaction commit after rollback"));
	}
	StorageClient::TransactionLock lock( m_storage);
	DocumentFrequencyCache::Batch cleaned_batch;
	DocumentFrequencyCache::Batch::iterator bi = m_dfbatch.begin(), be = m_dfbatch.end();
	for (; bi != be; ++bi)
	{
		if (bi->termno > UnknownValueHandleStart)
		{
			std::size_t termidx = m_unknownTerms[ bi->termno - UnknownValueHandleStart - 1];
			const char* termkey = m_unknownTerms_strings.c_str() + termidx;
			Index termno = m_dbadapter_termvalue.get( termkey);
			if (termno)
			{
				cleaned_batch.put( bi->typeno, termno, bi->value);
				bi->termno = termno;
			}
		}
		else
		{
			cleaned_batch.put( *bi);
		}
	}
	std::vector<DocumentFrequencyChange> rt;
	std::vector<NewTerm>::const_iterator ti = m_newTerms.begin(), te = m_newTerms.end();
	for (; ti != te; ++ti)
	{
		const DocumentFrequencyCache::Batch::Increment& incr = m_dfbatch[ ti->batchidx];
		if (incr.termno < UnknownValueHandleStart)
		{
			Index df = m_storage->localDocumentFrequency( incr.typeno, incr.termno);
			const char* value = m_newTerms_strings.c_str() + ti->termidx;
			const char* type = m_typeStrings[ incr.typeno].c_str();
			rt.push_back( DocumentFrequencyChange( type, value, df));
		}
	}

	m_documentFrequencyCache->writeBatch( cleaned_batch);
	m_storage->declareGlobalNofDocumentsInserted( m_nofDocumentsInserted);
	m_commit = true;
	m_nofDocumentsInserted = 0;

	return rt;
}

void PeerStorageTransaction::rollback()
{
	if (m_commit)
	{
		throw strus::runtime_error( _TXT( "called transaction rollback twice"));
	}
	if (m_rollback)
	{
		throw strus::runtime_error( _TXT( "called transaction rollback after commit"));
	}
	m_rollback = true;
}

