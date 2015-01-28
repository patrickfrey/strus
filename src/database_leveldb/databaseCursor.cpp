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
#include <cstring>
#include <stdexcept>

using namespace strus;

DatabaseCursor::DatabaseCursor( leveldb::DB* db_, bool useCache)
	:m_db(db_),m_itr(0)
{
	m_dboptions.fill_cache = useCache;
	m_itr = m_db->NewIterator( m_dboptions);
}

DatabaseCursor::~DatabaseCursor()
{
	delete m_itr;
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
	if (domainkeysize+1 > sizeof(m_domainkey))
	{
		throw std::runtime_error( "key domain prefix string exceeds maximum size allowed");
	}
	std::memcpy( m_domainkey, domainkey, m_domainkeysize=domainkeysize);
	m_domainkey[ domainkeysize] = (char)0xFF;
}

const char* DatabaseCursor::getCurrentKey() const
{
	if (checkDomain())
	{
		return m_itr->key().data();
	}
	else
	{
		return 0;
	}
}

const char* DatabaseCursor::seekUpperBound(
		const char* key,
		std::size_t keysize,
		std::size_t domainkeysize)
{
	initDomain( key, domainkeysize);
	m_itr->Seek( leveldb::Slice( key,keysize));
	return getCurrentKey();
}

const char* DatabaseCursor::seekFirst(
		const char* domainkey,
		std::size_t domainkeysize)
{
	initDomain( domainkey, domainkeysize);
	m_itr->Seek( leveldb::Slice( domainkey,domainkeysize));
	return getCurrentKey();
}

const char* DatabaseCursor::seekLast(
		const char* domainkey,
		std::size_t domainkeysize)
{
	initDomain( domainkey, domainkeysize);
	m_itr->Seek( leveldb::Slice( m_domainkey, m_domainkeysize+1));
	if (!m_itr->Valid())
	{
		m_itr->SeekToLast();
	}
	if (m_itr->Valid())
	{
		m_itr->Prev();
	}
	return getCurrentKey();
}

const char* DatabaseCursor::seekNext()
{
	if (m_itr->Valid())
	{
		m_itr->Next();
		return getCurrentKey();
	}
	return 0;
}

const char* DatabaseCursor::seekPrev()
{
	if (m_itr->Valid())
	{
		m_itr->Prev();
		return getCurrentKey();
	}
	return 0;
}


void DatabaseCursor::getValue(
			const char*& value,
			std::size_t& valuesize) const
{
	if (m_itr->Valid())
	{
		value = (const char*)m_itr->value().data();
		valuesize = m_itr->value().size();
	}
	else
	{
		value = 0;
		valuesize = 0;
	}
}

bool DatabaseCursor::getKeyValue(
			const char* key,
			std::size_t keysize,
			const char*& value,
			std::size_t& valuesize)
{
	leveldb::Status status = m_db->Get( m_dboptions, leveldb::Slice( key, keysize), &m_randomAccessValue);
	if (status.IsNotFound())
	{
		return false;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	value = m_randomAccessValue.c_str();
	valuesize = m_randomAccessValue.size();
	return true;
}




