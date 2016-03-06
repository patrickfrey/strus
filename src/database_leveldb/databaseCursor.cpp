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
#include "databaseCursor.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseCursor::DatabaseCursor( leveldb::DB* db_, bool useCache, bool useSnapshot, ErrorBufferInterface* errorhnd_)
	:m_db(db_),m_itr(0)
{
	if (useSnapshot)
	{
		m_dboptions.snapshot = m_db->GetSnapshot();
	}
	m_dboptions.fill_cache = useCache;
	m_itr = m_db->NewIterator( m_dboptions);
}

DatabaseCursor::~DatabaseCursor()
{
	delete m_itr;
	if (m_dboptions.snapshot)
	{
		m_db->ReleaseSnapshot( m_dboptions.snapshot);
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
		throw strus::runtime_error( _TXT( "key domain prefix string exceeds maximum size allowed"));
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
		const char* key,
		std::size_t keysize,
		std::size_t domainkeysize)
{
	try
	{
		initDomain( key, domainkeysize);
		m_itr->Seek( leveldb::Slice( key,keysize));
		return getCurrentKey();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error database cursor seek upper bound: %s"), *m_errorhnd, DatabaseCursorInterface::Slice());
}

DatabaseCursorInterface::Slice DatabaseCursor::seekUpperBoundRestricted(
		const char* key,
		std::size_t keysize,
		const char* upkey,
		std::size_t upkeysize)
{
	m_itr->Seek( leveldb::Slice( key,keysize));
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

DatabaseCursorInterface::Slice DatabaseCursor::seekFirst(
		const char* domainkey,
		std::size_t domainkeysize)
{
	try
	{
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
	if (m_itr->Valid())
	{
		return Slice( (const char*)m_itr->value().data(), m_itr->value().size());
	}
	else
	{
		return Slice();
	}
}



