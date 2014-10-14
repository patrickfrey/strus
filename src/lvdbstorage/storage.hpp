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
#ifndef _STRUS_LVDB_STORAGE_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_HPP_INCLUDED
#include "strus/storageInterface.hpp"
#include "strus/iteratorInterface.hpp"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <boost/thread/mutex.hpp>

namespace strus {

class Storage
	:public StorageInterface
{
public:
	Storage( const char* path_);
	virtual void close();
	virtual ~Storage();

	virtual IteratorInterface*
		createTermOccurrenceIterator(
			const std::string& termtype,
			const std::string& termid);

	virtual TransactionInterface*
		createTransaction( const std::string& docid);

	virtual Index nofDocumentsInserted() const;

	virtual Index maxDocumentNumber() const;

	virtual float documentAttributeNumeric( Index docno, char varname) const;

	virtual std::string documentAttributeString( Index docno, char varname) const;

public:
	void writeBatch(
		leveldb::WriteBatch& batch);
	void batchDefineVariable(
		leveldb::WriteBatch& batch, const char* name, Index value);

	leveldb::Iterator* newIterator();

	enum KeyPrefix
	{
		TermTypePrefix='t',	///< [type string]             ->  [typeno]
		TermValuePrefix='i',	///< [term string]             ->  [termno]
		DocIdPrefix='d',	///< [docid string]            ->  [docno]
		LocationPrefix='o',	///< [type,term,docno]         ->  [pos incr]*
		InversePrefix='r',	///< [docno,typeno,position]   ->  [term string]*
		VariablePrefix='v',	///< [variable string]         ->  [index]
		DocNumAttrPrefix='w',	///< [docno,nameid]            ->  [float]
		DocTextAttrPrefix='a'	///< [docno,nameid]            ->  [string]
	};

	static std::string keyString( KeyPrefix prefix, const std::string& keyname);
	Index keyLookUp( KeyPrefix prefix, const std::string& keyname);
	Index keyGetOrCreate( KeyPrefix prefix, const std::string& keyname);
	void flushNewKeys();

private:
	struct stlSliceComparator
	{
		bool operator()(const leveldb::Slice& a, const leveldb::Slice& b) const
		{
			return a.compare(b) < 0;
		}
	};
	typedef std::map<std::string,Index,stlSliceComparator> NewKeyMap;

private:
	std::string m_path;					///< levelDB storage path 
	leveldb::DB* m_db;					///< levelDB handle
	Index m_next_termno;					///< next index to assign to a new term value
	Index m_next_typeno;					///< next index to assign to a new term type
	Index m_next_docno;					///< next index to assign to a new document id
	Index m_nof_documents;					///< number of documents inserted
	boost::mutex m_mutex;					///< mutex for mutual exclusion for the access of counters (m_next_..) and for the access of keys not defined during a running transaction
	leveldb::WriteBatch m_newKeyBatch;			///< batch for new keys defined. flushed at end of every transaction
	NewKeyMap m_newKeyMap;					///< temporary map for the new keys defined
};

}
#endif


