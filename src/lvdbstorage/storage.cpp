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
		if (!!m_db)
		{
			delete m_db;
			m_db = 0;
		}
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}

void Storage::close()
{
	if (m_db)
	{
		leveldb::WriteBatch batch;
		batchDefineVariable( batch, "TermNo", m_next_termno);
		batchDefineVariable( batch, "TypeNo", m_next_typeno);
		batchDefineVariable( batch, "DocNo", m_next_docno);
	
		leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			throw std::runtime_error( std::string("error when flushing and closing storage") + status.ToString());
		}
		delete m_db;
		m_db = 0;
	}
}

Storage::~Storage()
{
	try
	{
		close();
	}
	catch (...)
	{
		//... silently ignored. Call close directly to catch errors
		if (m_db) delete m_db;
	}
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

Index Storage::curTermNo()
{
	return m_next_termno;
}

Index Storage::curTypeNo()
{
	return m_next_typeno;
}

Index Storage::curDocNo()
{
	return m_next_docno;
}

void Storage::writeBatch( leveldb::WriteBatch& batch)
{
	if (!m_db) throw std::runtime_error("write on closed storage");
	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
}

void Storage::batchDefineVariable( leveldb::WriteBatch& batch, const char* name, Index value)
{
	std::string encoded_value;
	packIndex( encoded_value, value);
	batch.Put( Storage::keyString( Storage::VariablePrefix, name), encoded_value);
}

leveldb::Iterator* Storage::newIterator()
{
	if (!m_db) throw std::runtime_error("open read iterator on closed storage");
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
	if (!m_db) throw std::runtime_error("read on closed storage");
	boost::mutex::scoped_lock( m_mutex);

	std::string key = keyString( prefix, keyname);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

Index Storage::keyGetOrCreate( KeyPrefix prefix, const std::string& keyname)
{
	if (!m_db) throw std::runtime_error("read on closed storage");
	std::string key = keyString( prefix, keyname);
	leveldb::Slice constkey( key.c_str(), key.size());
	std::string value;
	leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
	if (status.IsNotFound())
	{
		boost::mutex::scoped_lock( m_mutex);
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), constkey, &value);
		if (status.IsNotFound())
		{
			if (prefix == TermTypePrefix)
			{
				return m_next_typeno++;
			}
			else if (prefix == TermValuePrefix)
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
		else if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
	}
	else if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	const char* cc = value.c_str();
	return unpackIndex( cc, cc + value.size());
}

IteratorInterface*
	Storage::createTermOccurrenceIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( TermTypePrefix, typestr);
	Index termno = keyLookUp( TermValuePrefix, termstr);
	return new Iterator( m_db, typeno, termno);
}

StorageInterface::TransactionInterface*
	Storage::createTransaction(
		const std::string& docid)
{
	boost::mutex::scoped_lock( m_mutex);
	return new Transaction( this, docid);
}

