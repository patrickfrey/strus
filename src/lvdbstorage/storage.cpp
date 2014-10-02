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
#include "storage.hpp"
#include "iterator.hpp"
#include "transaction.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>

using namespace strus;

Storage::Storage( const char* path_)
	:m_path(path_),m_db(0)
{
	leveldb::Options options;
	options.create_if_missing = false;

	leveldb::Status status = leveldb::DB::Open(options, path_, &m_db);
	if (status.ok())
	{
		m_next_termno = keyLookUp( VariablePrefix, "TermNo");
		m_next_typeno = keyLookUp( VariablePrefix, "TypeNo");
		m_next_docno = keyLookUp( VariablePrefix, "DocNo");
	}
	else
	{
		std::string err = status.ToString();
		if (!m_db)
		{
			delete m_db;
			m_db = 0;
		}
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}

Storage::~Storage()
{
	if (m_db) delete m_db;
}

Index Storage::newTermNo()
{
	boost::mutex::scoped_lock( m_mutex);
	return m_next_termno++;
}

Index Storage::newTypeNo()
{
	boost::mutex::scoped_lock( m_mutex);
	return m_next_typeno++;
}

Index Storage::newDocNo()
{
	boost::mutex::scoped_lock( m_mutex);
	return m_next_docno++;

}

void Storage::writeBatch( leveldb::WriteBatch& batch)
{
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
}

leveldb::Iterator* Storage::newIterator()
{
	return m_db->NewIterator( leveldb::ReadOptions());
}



std::string Storage::keyString( KeyPrefix prefix, const std::string& keyname)
{
	std::string rt;
	rt.push_back( (char)prefix);
	rt.append( keyname);
	return rt;
}

Index Storage::keyLookUp( KeyPrefix prefix, const std::string& keyname)
{
	boost::mutex::scoped_lock( m_mutex);

	std::string key = keyString( prefix, keyname);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	if (status.IsNotFound()) return 0;
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

Index Storage::keyGetOrCreate( KeyPrefix prefix, const std::string& keyname)
{
	std::string key = keyString( prefix, keyname);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	if (status.IsNotFound())
	{
		boost::mutex::scoped_lock( m_mutex);
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
		if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
		if (status.IsNotFound())
		{
			if (prefix == TypeIdPrefix)
			{
				return m_next_typeno++;
			}
			else if (prefix == TermIdPrefix)
			{
				return m_next_termno++;
			}
			else if (prefix == DocIdPrefix)
			{
				return m_next_docno++;
			}
			else
			{
				throw std::logic_error( "internal: Cannot create index value");
			}
		}
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

IteratorInterface*
	Storage::createTermOccurrenceIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( TermIdPrefix, typestr);
	Index termno = keyLookUp( TermIdPrefix, termstr);
	return new Iterator( m_db, typeno, termno);
}

StorageInterface::TransactionInterface*
	Storage::createTransaction(
		const std::string& docid)
{
	boost::mutex::scoped_lock( m_mutex);
	return new Transaction( this, docid);
}

