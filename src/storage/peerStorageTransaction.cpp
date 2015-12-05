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
#include "strus/peerMessageViewerInterface.hpp"
#include "strus/peerMessageBuilderInterface.hpp"
#include "strus/peerMessageProcessorInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "storageClient.hpp"

using namespace strus;

PeerStorageTransaction::PeerStorageTransaction( StorageClient* storage_, DatabaseClientInterface* database_, DocumentFrequencyCache* dfcache_, const PeerMessageProcessorInterface* peermsgproc_, ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_documentFrequencyCache(dfcache_)
	,m_peermsgproc(peermsgproc_)
	,m_dbadapter_termvalue(database_)
	,m_termvaluecnt(0)
	,m_nofDocumentsInserted(0)
	,m_commit(false)
	,m_errorhnd(errorhnd_)
{}

void PeerStorageTransaction::clear()
{
	m_dfbatch.clear();
	m_unknownTerms.clear();
	m_unknownTerms_strings.clear();
	m_newTerms.clear();
	m_newTerms_strings.clear();
	m_typeStrings.clear();
	m_termvaluecnt = 0;
	m_nofDocumentsInserted = 0;
	m_commit = false;
	m_returnblob.clear();
}

void PeerStorageTransaction::updateNofDocumentsInsertedChange( const GlobalCounter& increment)
{
	m_nofDocumentsInserted += increment;
}

void PeerStorageTransaction::updateDocumentFrequencyChange(
		const char* termtype, const char* termvalue, const GlobalCounter& increment, bool isNew)
{
	bool typeno_isNew = false;
	Index typeno = m_storage->allocTypenoImm( termtype, typeno_isNew);
	Index termno = m_dbadapter_termvalue.get( termvalue);
	if (!termno)
	{
		m_unknownTerms_strings.push_back( termvalue);
		m_unknownTerms.push_back( m_unknownTerms_strings.back());

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
			m_newTerms_strings.push_back( termvalue);
			m_newTerms.push_back( NewTerm( m_newTerms_strings.back(), m_dfbatch.size()));
		}
	}
	m_dfbatch.put( typeno, termno, increment);
}

void PeerStorageTransaction::push( const char* inmsg, std::size_t inmsgsize, bool sign)
{
	try
	{
		if (m_commit)
		{
			m_errorhnd->report( _TXT( "called peer storage transaction push after commit"));
		}
		std::auto_ptr<PeerMessageViewerInterface> viewer( m_peermsgproc->createViewer( inmsg, inmsgsize));
		if (!viewer.get()) throw strus::runtime_error( _TXT( "error creating peer message viewer"));

		PeerMessageViewerInterface::DocumentFrequencyChange rec;
		while (viewer->nextDfChange( rec))
		{
			updateDocumentFrequencyChange( rec.type, rec.value, sign?rec.increment:-rec.increment, rec.isnew);
		}
		GlobalCounter nofdocs = viewer->nofDocumentsInsertedChange();
		updateNofDocumentsInsertedChange( sign?nofdocs:-nofdocs);
	}
	CATCH_ERROR_MAP( _TXT("error in peer storage transaction push: %s"), *m_errorhnd);
}

void PeerStorageTransaction::rollback()
{
	clear();
}

bool PeerStorageTransaction::commit( const char*& outmsg, std::size_t& outmsgsize)
{
	try
	{
		if (m_errorhnd->hasError())
		{
			m_errorhnd->explain(_TXT("peer storage transaction commit failed: %s"));
			return false;
		}
		if (m_commit)
		{
			throw strus::runtime_error( _TXT( "called peer storage transaction commit twice"));
		}
		m_returnblob.clear();
		StorageClient::TransactionLock lock( m_storage);
		DocumentFrequencyCache::Batch cleaned_batch;
		DocumentFrequencyCache::Batch::iterator bi = m_dfbatch.begin(), be = m_dfbatch.end();
		for (; bi != be; ++bi)
		{
			if (bi->termno > UnknownValueHandleStart)
			{
				const char* termkey = m_unknownTerms[ bi->termno - UnknownValueHandleStart - 1];
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
		PeerMessageProcessorInterface::BuilderOptions options;
		std::auto_ptr<PeerMessageBuilderInterface> msgbuilder( m_peermsgproc->createBuilder( options));
		if (!msgbuilder.get()) throw strus::runtime_error( _TXT( "error creating peer message builder"));

		std::vector<NewTerm>::const_iterator ti = m_newTerms.begin(), te = m_newTerms.end();
		for (; ti != te; ++ti)
		{
			const DocumentFrequencyCache::Batch::Increment& incr = m_dfbatch[ ti->batchidx];
			if (incr.termno < UnknownValueHandleStart)
			{
				Index df = m_storage->localDocumentFrequency( incr.typeno, incr.termno);
				const char* value = ti->term;
				const char* type = m_typeStrings[ incr.typeno].c_str();
				msgbuilder->addDfChange( type, value, df, false);
			}
		}
		const char* blk;
		std::size_t blksize;
		while (msgbuilder->fetchMessage( blk, blksize))
		{
			m_returnblob.append( blk, blksize);
		}
		if( m_errorhnd->hasError())
		{
			m_errorhnd->explain( _TXT("error in peer message transaction: %s"));
			return false;
		}
		m_documentFrequencyCache->writeBatch( cleaned_batch);
		m_storage->declareGlobalNofDocumentsInserted( m_nofDocumentsInserted);
		m_commit = true;
		m_nofDocumentsInserted = 0;
		outmsg = m_returnblob.c_str();
		outmsgsize = m_returnblob.size();
		return true;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in peer storage transaction commit: %s"), *m_errorhnd, false);
}



