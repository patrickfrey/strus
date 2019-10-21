/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DATABASE_CURSOR_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_DATABASE_CURSOR_IMPLEMENTATION_HPP_INCLUDED
#include "strus/databaseCursorInterface.hpp"
#include "levelDbHandle.hpp"
#include "strus/base/shared_ptr.hpp"
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
	DatabaseCursor( strus::shared_ptr<LevelDbConnection> conn_, bool useCache, bool useSnapshot, ErrorBufferInterface* errorhnd_);

	virtual ~DatabaseCursor();

	virtual Slice seekUpperBound(
			const char* keystr,
			std::size_t keysize,
			std::size_t domainkeysize);

	virtual Slice seekUpperBoundRestricted(
			const char* keystr,
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
	DatabaseCursor( DatabaseCursor&){}			///... uncopyable
	void operator=( DatabaseCursor&){}			///... uncopyable

private:
	bool checkDomain() const;
	void initDomain( const char* domainkey, std::size_t domainkeysize);
	Slice getCurrentKey() const;

private:
	LevelDbIterator m_itrhnd;				///< handle for iterator on levelDB blocks
	enum {MaxDomainKeySize=32};
	unsigned char m_domainkey[ MaxDomainKeySize];		///< key prefix defining the current domain to scan
	std::size_t m_domainkeysize;				///< size of domain key in bytes
	std::string m_randomAccessValue;			///< buffer for value retrieved with getKeyValue
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}//namespace
#endif


