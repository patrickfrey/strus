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
#include "documentFrequencyCache.hpp"
#include "databaseAdapter.hpp"
#include "private/stringMap.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class StorageClient;
/// \brief Forward declaration
class DocumentFrequencyCache;
/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class PeerMessageProcessorInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Interface for a transaction of global statistic changes
class PeerStorageTransaction
{
public:
	PeerStorageTransaction( StorageClient* storage_, DatabaseClientInterface* database_, DocumentFrequencyCache* dfcache_, const PeerMessageProcessorInterface* peermsgproc_, ErrorBufferInterface* errorhnd_);
	~PeerStorageTransaction(){}

	std::string run( const char* msg, std::size_t msgsize);

private:
	void clear();

	void updateNofDocumentsInsertedChange( const GlobalCounter& increment);
	void updateDocumentFrequencyChange(
			const char* termtype, const char* termvalue, const GlobalCounter& increment, bool isNew);

	enum {
		UnknownValueHandleStart=(1<<30)
	};

	struct NewTerm
	{
	public:
		NewTerm( const char* term_, std::size_t batchidx_)
			:term(term_),batchidx(batchidx_){}
		NewTerm( const NewTerm& o)
			:term(o.term),batchidx(o.batchidx){}

		const char* term;
		std::size_t batchidx;
	};

private:
	StorageClient* m_storage;
	DatabaseClientInterface* m_database;
	DocumentFrequencyCache* m_documentFrequencyCache;
	const PeerMessageProcessorInterface* m_peermsgproc;
	DocumentFrequencyCache::Batch m_dfbatch;
	std::vector<const char*> m_unknownTerms;
	StringVector m_unknownTerms_strings;
	std::vector<NewTerm> m_newTerms;
	StringVector m_newTerms_strings;
	std::vector<std::string> m_typeStrings;
	DatabaseAdapter_TermValue::ReadWriter m_dbadapter_termvalue;
	Index m_termvaluecnt;
	GlobalCounter m_nofDocumentsInserted;
	bool m_commit;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif

