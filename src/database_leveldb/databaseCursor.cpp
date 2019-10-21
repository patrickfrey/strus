/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "databaseCursor.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/shared_ptr.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

#define MODULENAME "DatabaseCursor"

static leveldb::ReadOptions getOptions( const strus::shared_ptr<LevelDbConnection>& conn_, bool useCache, bool useSnapshot)
{
	leveldb::ReadOptions rt;
	leveldb::DB* db = conn_->db();
	if (useSnapshot)
	{
		rt.snapshot = db->GetSnapshot();
	}
	rt.fill_cache = useCache;
	return rt;
}

DatabaseCursor::DatabaseCursor( strus::shared_ptr<LevelDbConnection> conn_, bool useCache, bool useSnapshot, ErrorBufferInterface* errorhnd_)
	:m_itrhnd(getOptions(conn_,useCache,useSnapshot),conn_),m_itr(0),m_domainkeysize(0),m_randomAccessValue(),m_errorhnd(errorhnd_)
{
	m_itr = m_itrhnd.itr();
}

DatabaseCursor::~DatabaseCursor()
{
	m_itrhnd.done();
	if (m_itrhnd.opt().snapshot)
	{
		leveldb::DB* db = m_itrhnd.db();
		if (db) db->ReleaseSnapshot( m_itrhnd.opt().snapshot);
	}
}

bool DatabaseCursor::checkDomain() const
{
	if (m_itr->Valid()
	&&  m_domainkeysize <= m_itr->key().size()
	&&  0==std::memcmp( m_domainkey, m_itr->key().data(), m_domainkeysize))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void DatabaseCursor::initDomain( const char* domainkey, std::size_t domainkeysize)
{
	if (domainkeysize+1 >= sizeof(m_domainkey))
	{
		throw std::runtime_error( _TXT( "key domain prefix string exceeds maximum size allowed"));
	}
	std::memcpy( m_domainkey, domainkey, m_domainkeysize=domainkeysize);
	m_domainkey[ m_domainkeysize] = 0xFF;
}

DatabaseCursorInterface::Slice DatabaseCursor::getCurrentKey() const
{
	if (checkDomain())
	{
		return Slice( m_itr->key().data(), m_itr->key().size());
	}
	else
	{
		return Slice();
	}
}

DatabaseCursorInterface::Slice DatabaseCursor::seekUpperBound(
		const char* keystr,
		std::size_t keysize,
		std::size_t domainkeysize)
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		initDomain( keystr, domainkeysize);
		m_itr->Seek( leveldb::Slice( keystr, keysize));
		return getCurrentKey();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek upper bound: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekUpperBoundRestricted(
		const char* keystr,
		std::size_t keysize,
		const char* upkey,
		std::size_t upkeysize)
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		m_itr->Seek( leveldb::Slice( keystr,keysize));
		if (m_itr->Valid())
		{
			std::size_t kk = upkeysize < keysize ? upkeysize : keysize;
			int res = std::memcmp( m_itr->key().data(), upkey, kk);
			if (res < 0 || (res == 0 && upkeysize < keysize))
			{
				return Slice( m_itr->key().data(), m_itr->key().size());
			}
		}
		return Slice();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek upper bound restricted: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekFirst(
		const char* domainkey,
		std::size_t domainkeysize)
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		initDomain( domainkey, domainkeysize);
		m_itr->Seek( leveldb::Slice( domainkey,domainkeysize));
		return getCurrentKey();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek first: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekLast(
		const char* domainkey,
		std::size_t domainkeysize)
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		initDomain( domainkey, domainkeysize);
		if (m_domainkeysize == 0)
		{
			m_itr->SeekToLast();
		}
		else
		{
			m_itr->Seek( leveldb::Slice( (char*)m_domainkey, m_domainkeysize+1));
			if (m_itr->Valid())
			{
				m_itr->Prev();
			}
			else
			{
				return Slice();
			}
		}
		return getCurrentKey();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek last: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekNext()
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		if (m_itr->Valid())
		{
			m_itr->Next();
			return getCurrentKey();
		}
		return Slice();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek next: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekPrev()
{
	try
	{
		leveldb::DB* db = m_itrhnd.db();
		if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

		if (m_itr->Valid())
		{
			m_itr->Prev();
			return getCurrentKey();
		}
		return Slice();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek previous: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::key() const
{
	leveldb::DB* db = m_itrhnd.db();
	if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

	if (m_itr->Valid())
	{
		return Slice( (const char*)m_itr->key().data(), m_itr->key().size());
	}
	else
	{
		return Slice();
	}
}

DatabaseCursorInterface::Slice DatabaseCursor::value() const
{
	leveldb::DB* db = m_itrhnd.db();
	if (!db) throw strus::runtime_error(_TXT("called method '%s::%s' after close"), MODULENAME, "seekUpperBound");

	if (m_itr->Valid())
	{
		return Slice( (const char*)m_itr->value().data(), m_itr->value().size());
	}
	else
	{
		return Slice();
	}
}



