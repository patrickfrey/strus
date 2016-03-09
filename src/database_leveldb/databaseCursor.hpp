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
#ifndef _STRUS_DATABASE_CURSOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_CURSOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseCursorInterface.hpp"
#include <string>
#include <leveldb/db.h>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Implementation of the DatabaseCursorInterface based on the LevelDB library
class DatabaseCursor
	:public DatabaseCursorInterface
{
public:
	DatabaseCursor( leveldb::DB* db_, bool useCache, bool useSnapshot, ErrorBufferInterface* errorhnd_);

	virtual ~DatabaseCursor();

	virtual Slice seekUpperBound(
			const char* key,
			std::size_t keysize,
			std::size_t domainkeysize);

	virtual Slice seekUpperBoundRestricted(
			const char* key,
			std::size_t keysize,
			const char* upkey,
			std::size_t upkeysize);

	virtual Slice seekFirst(
			const char* domainkey,
			std::size_t domainkeysize);

	virtual Slice seekLast(
			const char* domainkey,
			std::size_t domainkeysize);

	virtual Slice seekNext();

	virtual Slice seekPrev();

	virtual Slice key() const;

	virtual Slice value() const;

private:
	bool checkDomain() const;
	void initDomain( const char* domainkey, std::size_t domainkeysize);
	Slice getCurrentKey() const;

private:
	leveldb::DB* m_db;					///< levelDB handle
	leveldb::ReadOptions m_dboptions;			///< options for levelDB
	leveldb::Iterator* m_itr;				///< iterator on levelDB blocks
	enum {MaxDomainKeySize=32};
	unsigned char m_domainkey[ MaxDomainKeySize];		///< key prefix defining the current domain to scan
	std::size_t m_domainkeysize;				///< size of domain key in bytes
	std::string m_randomAccessValue;			///< buffer for value retrieved with getKeyValue
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif


