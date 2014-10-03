/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
	virtual ~Storage();

	virtual IteratorInterface*
		createTermOccurrenceIterator(
			const std::string& termtype,
			const std::string& termid);

	virtual TransactionInterface*
		createTransaction( const std::string& docid);

	Index newTermNo();
	Index newTypeNo();
	Index newDocNo();

	void writeBatch( leveldb::WriteBatch& batch);

	leveldb::Iterator* newIterator();

	enum KeyPrefix
	{
		TermTypePrefix='t',	///< [type string]      ->  [typeno]
		TermValuePrefix='i',	///< [term string]      ->  [termno]
		DocIdPrefix='d',	///< [docid string]     ->  [docno]
		LocationPrefix='o',	///< [type,term,docno]  ->  [pos incr]*
		InversePrefix='r',	///< [docno,position]   ->  [typeno,termno]*
		VariablePrefix='v'	///< [variable string]  ->  [index]
	};

	static std::string keyString( KeyPrefix prefix, const std::string& keyname);
	Index keyLookUp( KeyPrefix prefix, const std::string& keyname);
	Index keyGetOrCreate( KeyPrefix prefix, const std::string& keyname);

private:
	std::string m_path;
	leveldb::DB* m_db;
	Index m_next_termno;
	Index m_next_typeno;
	Index m_next_docno;
	boost::mutex m_mutex;
};

}
#endif


