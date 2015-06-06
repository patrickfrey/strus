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
#include "databaseCursor.hpp"
#include "private/internationalization.hpp"
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseCursor::DatabaseCursor( leveldb::DB* db_, bool useCache, bool useSnapshot)
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
	initDomain( key, domainkeysize);
	m_itr->Seek( leveldb::Slice( key,keysize));
	return getCurrentKey();
}

DatabaseCursorInterface::Slice DatabaseCursor::seekFirst(
		const char* domainkey,
		std::size_t domainkeysize)
{
	initDomain( domainkey, domainkeysize);
	m_itr->Seek( leveldb::Slice( domainkey,domainkeysize));
	return getCurrentKey();
}

DatabaseCursorInterface::Slice DatabaseCursor::seekLast(
		const char* domainkey,
		std::size_t domainkeysize)
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

DatabaseCursorInterface::Slice DatabaseCursor::seekNext()
{
	if (m_itr->Valid())
	{
		m_itr->Next();
		return getCurrentKey();
	}
	return Slice();
}

DatabaseCursorInterface::Slice DatabaseCursor::seekPrev()
{
	if (m_itr->Valid())
	{
		m_itr->Prev();
		return getCurrentKey();
	}
	return Slice();
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



