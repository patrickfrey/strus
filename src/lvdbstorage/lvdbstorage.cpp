#include "strus/storageInterface.hpp"
#include "indexPacker.hpp"

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
#include "strus/storageInterface.hpp"
#include "dll_tags.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <boost/thread/mutex.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

using namespace strus;

class StorageLevelDb
	:public StorageInterface
{
public:
	StorageLevelDb( const char* path_);
	virtual ~StorageLevelDb();

	virtual IteratorInterface*
		createTermOccurrenceIterator(
			const std::string& termtype,
			const std::string& termid);

	virtual Transaction*
		createTransaction( const std::string& docid);

	Index newTermNo()
	{
		boost::mutex::scoped_lock( m_mutex);
		return m_next_termno++;
	}

	Index newTypeNo()
	{
		boost::mutex::scoped_lock( m_mutex);
		return m_next_typeno++;
	}

	Index newDocNo()
	{
		boost::mutex::scoped_lock( m_mutex);
		return m_next_docno++;

	}

	void writeBatch( leveldb::WriteBatch& batch)
	{
		leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
	}

	leveldb::Iterator* newIterator()
	{
		return m_db->NewIterator( leveldb::ReadOptions());
	}

	enum KeyPrefix
	{
		TypeIdPrefix='t',	///< [type string]      ->  [typeno]
		TermIdPrefix='i',	///< [term string]      ->  [termno]
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


StorageLevelDb::StorageLevelDb( const char* path_)
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


std::string StorageLevelDb::keyString( KeyPrefix prefix, const std::string& keyname)
{
	std::string rt;
	rt.push_back( (char)prefix);
	rt.append( keyname);
	return rt;
}

Index StorageLevelDb::keyLookUp( KeyPrefix prefix, const std::string& keyname)
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

Index StorageLevelDb::keyGetOrCreate( KeyPrefix prefix, const std::string& keyname)
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

StorageLevelDb::~StorageLevelDb()
{
	if (m_db) delete m_db;
}

class IteratorLevelDb
	:public IteratorInterface
{
public:
	IteratorLevelDb( leveldb::DB* db_, Index termtypeno, Index termidno)
		:m_db(db_),m_itr(0),m_posno(0),m_positr(0),m_posend(0)
	{
		m_key.push_back( (char)StorageLevelDb::LocationPrefix);
		packIndex( m_key, termtypeno);
		packIndex( m_key, termidno);
		m_keysize = m_key.size();
	}

	virtual ~IteratorLevelDb()
	{
		if (m_itr) delete m_itr;
	}


	virtual Index skipDoc( const Index& docno)
	{
		if (m_itr && m_docno +1 == docno)
		{
			return getNextTermDoc();
		}
		else
		{
			return getFirstTermDoc( docno);
		}
	}

	virtual Index skipPos( const Index& firstpos)
	{
		if (m_posno > firstpos)
		{
			m_posno = 0;
			m_positr = m_itr->value().data();
			m_posend = m_positr + m_itr->value().size();
		}
		while (m_positr != m_posend && firstpos > m_posno)
		{
			// Get the next position increment and with it the next position number:
			Index incr = unpackIndex( m_positr, m_posend);
			m_posno += incr;
		}
		if (firstpos > m_posno)
		{
			return 0;
		}
		return m_posno;
	}

private:
	Index extractMatchDocno()
	{
		if (m_keysize < m_itr->key().size() && 0==std::memcmp( m_key.c_str(), m_itr->key().data(), m_keysize))
		{
			// Check if we are still on the same term:
			const char* ki = m_itr->key().data();
			const char* ke = ki + m_itr->key().size();

			m_posno = 0;
			m_positr = m_itr->value().data();
			m_posend = m_positr + m_itr->value().size();

			// Return the matching document number:
			return m_docno=unpackIndex( ki, ke);
		}
		else
		{
			delete m_itr;
			m_docno = 0;
			m_itr = 0;
			m_posno = 0;
			m_positr = 0;
			m_posend = 0;
			return 0;
		}
	}

	Index getNextTermDoc()
	{
		m_itr->Next();
		return extractMatchDocno();
	}

	Index getFirstTermDoc( const Index& docno)
	{
		if (!m_itr)
		{
			m_itr = m_db->NewIterator( leveldb::ReadOptions());
		}
		m_key.resize( m_keysize);
		packIndex( m_key, docno);
		m_itr->Seek( leveldb::Slice( m_key.c_str(), m_keysize));

		return extractMatchDocno();
	}

private:
	leveldb::DB* m_db;
	std::string m_key;
	std::size_t m_keysize;
	Index m_docno;
	leveldb::Iterator* m_itr;
	Index m_posno;
	const char* m_positr;
	const char* m_posend;
};

/// \remark More than one transactions at the same time are possible, but will slow down the system
class TransactionLevelDb
	:public StorageInterface::Transaction
{
public:
	TransactionLevelDb( StorageLevelDb* storage_, const std::string& docid_)
		:m_storage(storage_),m_docid(docid_)
	{
	}

	virtual ~TransactionLevelDb()
	{
		//... nothing done here. The document id and term or type ids 
		//	created might remain inserted, even after a rollback.
	}

	virtual void addTermOccurrence(
			const std::string& type_,
			const std::string& id_,
			const Index& position_)
	{
		if (position_ == 0) throw std::runtime_error( "term occurrence position must not be 0");

		Index typeno = m_storage->keyGetOrCreate( StorageLevelDb::TypeIdPrefix, id_);
		Index termno = m_storage->keyGetOrCreate( StorageLevelDb::TermIdPrefix, id_);

		std::vector<Index>* termpos = &m_terms[ TermMapKey( typeno, termno)];
		if (termpos->size())
		{
			if (termpos->back() == position_)
			{
				return; // ... ignoring multiple matches
			}
		}
		else if (termpos->back() < position_)
		{
			termpos->push_back( position_);
		}
		else
		{
			std::string* encterm = &m_invs[ position_];
			packIndex( *encterm, typeno);
			packIndex( *encterm, termno);

			std::vector<Index>::iterator pi = termpos->begin(), pe = termpos->end();
			for (; pi != pe; ++pi)
			{
				if (*pi >= position_)
				{
					if (*pi > position_)
					{
						// ... ignoring (*pi == position_) multiple matches
						// inserting match:
						termpos->insert( pi, position_);
					}
					return;
				}
			}
		}
	}

	virtual void commit()
	{
		Index docno = m_storage->keyGetOrCreate( StorageLevelDb::DocIdPrefix, m_docid);
		leveldb::WriteBatch batch;

		// Delete old document term occurrencies:
		std::map< TermMapKey, bool > oldcontent;
		std::string invkey;
		invkey.push_back( (char)StorageLevelDb::InversePrefix);
		packIndex( invkey, docno);

		leveldb::Iterator* vi = m_storage->newIterator();
		for (vi->Seek( invkey); vi->Valid(); vi->Next())
		{
			if (invkey.size() > vi->key().size() || 0!=std::strcmp( vi->key().data(), invkey.c_str()))
			{
				//... end of document reached
				break;
			}
			batch.Delete( vi->key());

			const char* di = vi->value().data();
			const char* de = di + vi->value().size();
			while (di != de)
			{
				Index typeno = unpackIndex( di, de);
				Index termno = unpackIndex( di, de);
				oldcontent[ TermMapKey( typeno, termno)] = true;
			}
		}
		std::map< TermMapKey, bool >::const_iterator di = oldcontent.begin(), de = oldcontent.end();
		for (; di != de; ++di)
		{
			std::string delkey;
			delkey.push_back( (char)StorageLevelDb::LocationPrefix);
			packIndex( delkey, di->first.first);
			packIndex( delkey, di->first.second);
			packIndex( delkey, docno);
			batch.Delete( delkey);
		}

		// Insert the new terms:
		TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
		for (; ti != te; ++ti)
		{
			std::string termkey;
			std::string positions;
			termkey.push_back( (char)StorageLevelDb::LocationPrefix);
			packIndex( termkey, ti->first.first);
			packIndex( termkey, ti->first.second);
			packIndex( termkey, docno);
			std::vector<Index>::const_iterator pi = ti->second.begin(), pe = ti->second.end();
			Index previous_pos = 0;
			for (; pi != pe; ++pi)
			{
				packIndex( positions, *pi - previous_pos);
				previous_pos = *pi;
			}
			batch.Put( termkey, positions);
		}

		// Insert the new inverted info:
		InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
		for (; ri != re; ++ri)
		{
			invkey.clear();
			invkey.push_back( (char)StorageLevelDb::InversePrefix);
			packIndex( invkey, docno);
			packIndex( invkey, ri->first);

			batch.Put( invkey, ri->second);
		}

		// Do submit the write to the database:
		m_storage->writeBatch( batch);
	}

private:
	typedef std::pair<Index,Index> TermMapKey;
	typedef std::map< TermMapKey, std::vector<Index> > TermMap;
	typedef std::map< Index, std::string > InvMap;

private:
	StorageLevelDb* m_storage;
	std::string m_docid;
	TermMap m_terms;
	InvMap m_invs;
};


IteratorInterface*
	StorageLevelDb::createTermOccurrenceIterator(
		const std::string& typestr,
		const std::string& termstr)
{
	Index typeno = keyLookUp( TermIdPrefix, typestr);
	Index termno = keyLookUp( TermIdPrefix, termstr);
	return new IteratorLevelDb( m_db, typeno, termno);
}

StorageInterface::Transaction*
	StorageLevelDb::createTransaction(
		const std::string& docid)
{
	boost::mutex::scoped_lock( m_mutex);
	return new TransactionLevelDb( this, docid);
}


static const char* configGet( const char* config, const char* name)
{
	const char* cc = config;
	std::size_t namelen = std::strlen(name);
	while (0!=std::strcmp( cc, name) || cc[namelen] != '=')
	{
		cc = std::strchr( cc, ';');
		if (!cc) break;
		cc = cc + 1;
	}
	return 0;
}

DLL_PUBLIC StorageInterface* createStorageClient( const char* config)
{
	const char* name = configGet( config, "name");
	if (!name)
	{
		throw std::runtime_error( "no storage name defined for kcstorage");
	}
	return new StorageLevelDb( name);
}

DLL_PUBLIC void createStorageDatabase( const char* config)
{
	const char* path = configGet( config, "path");
	if (!path)
	{
		throw std::runtime_error( "no storage name defined for storage");
	}
	leveldb::DB* db = 0;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = true;
	leveldb::Status status = leveldb::DB::Open( options, path, &db);
	if (status.ok())
	{
		leveldb::WriteBatch batch;
		batch.Put( StorageLevelDb::keyString( StorageLevelDb::VariablePrefix, "TermNo"), "\1");
		batch.Put( StorageLevelDb::keyString( StorageLevelDb::VariablePrefix, "TypeNo"), "\1");
		batch.Put( StorageLevelDb::keyString( StorageLevelDb::VariablePrefix, "DocNo"), "\1");
		status = db->Write( leveldb::WriteOptions(), &batch);
		if (!status.ok())
		{
			(void)leveldb::DestroyDB( path, leveldb::Options());
		}

	}
	if (!status.ok())
	{
		std::string err = status.ToString();
		if (!db)
		{
			(void)leveldb::DestroyDB( path, leveldb::Options());
			delete db;
		}
		throw std::runtime_error( std::string( "failed to create storage: ") + err);
	}
}




