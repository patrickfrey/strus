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
#ifndef _STRUS_PEER_STORAGE_TRANSACTION_HPP_INCLUDED
#define _STRUS_PEER_STORAGE_TRANSACTION_HPP_INCLUDED
#include "strus/peerStorageTransactionInterface.hpp"
#include "documentFrequencyCache.hpp"
#include "databaseAdapter.hpp"
#include <string>
#include <vector>
#include <boost/thread/mutex.hpp>

namespace strus
{

/// \brief Forward declaration
class Storage;
/// \brief Forward declaration
class DocumentFrequencyCache;
/// \brief Forward declaration
class DatabaseInterface;

/// \brief Interface for a transaction of global statistic changes
class PeerStorageTransaction
	:public PeerStorageTransactionInterface
{
public:
	PeerStorageTransaction( Storage* storage_, DatabaseInterface* database_, DocumentFrequencyCache* dfcache_);
	virtual ~PeerStorageTransaction();

	virtual void updateNofDocumentsInsertedChange( const GlobalCounter& increment);

	virtual void updateDocumentFrequencyChange(
			const char* termtype, const char* termvalue, const GlobalCounter& increment);

	virtual void commit();

	virtual void rollback();

private:
	enum {
		UnknownValueHandleStart=(1<<30)
	};

private:
	Storage* m_storage;
	DatabaseInterface* m_database;
	DocumentFrequencyCache* m_documentFrequencyCache;
	DocumentFrequencyCache::Batch m_dfbatch;
	std::vector<std::string> m_unknownTerms;
	DatabaseAdapter_TermValue m_dbadapter_termvalue;
	Index m_termvaluecnt;
	GlobalCounter m_nofDocumentsInserted;
	bool m_commit;
	bool m_rollback; 
};
}//namespace
#endif

