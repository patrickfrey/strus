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
#include "keyValueStorage.hpp"
#include <boost/scoped_ptr.hpp>
#include <cstring>

using namespace strus;

const KeyValueStorage::Value* KeyValueStorage::load( const Key& key)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Status status = m_db->Get( m_readOptions, keyslice, &m_curvaluestr);
	if (status.IsNotFound())
	{
		return 0;
	}
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	m_curvalue.init( m_curvaluestr.c_str(), m_curvaluestr.size());
	return &m_curvalue;
}

void KeyValueStorage::store( const Key& key, const Value& value, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());
	leveldb::Slice valueslice( value.ptr(), value.size());

	batch.Put( keyslice, valueslice);
}

void KeyValueStorage::dispose( const Key& key, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());

	batch.Delete( keyslice);
}

template <class Action>
void doForeach( leveldb::DB* db, strus::DatabaseKey::KeyPrefix keyprefix, const KeyValueStorage::Key& key, Action& action)
{
	leveldb::Iterator* vi = db->NewIterator( leveldb::ReadOptions());
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	std::string keystr;
	keystr.push_back( (char)keyprefix);
	keystr.append( key.str());

	leveldb::Slice keyslice( keystr.c_str(), keystr.size());

	for (vi->Seek( keyslice); vi->Valid(); vi->Next())
	{
		if (keystr.size() > vi->key().size()
		||  0!=std::memcmp( vi->key().data(), keystr.c_str(), keystr.size()))
		{
			//... end of document reached
			break;
		}
		action( vi->key(), vi->value());
	}
}

struct DeleteNodeAction
{
	DeleteNodeAction( leveldb::WriteBatch& batch_)
		:batch(&batch_){}
	DeleteNodeAction( const DeleteNodeAction& o)
		:batch(o.batch){}

	void operator()( const leveldb::Slice& keyslice, const leveldb::Slice&)
	{
		batch->Delete( keyslice);
	}

private:
	leveldb::WriteBatch* batch;
};

void KeyValueStorage::disposeSubnodes( const Key& key, leveldb::WriteBatch& batch)
{
	DeleteNodeAction action( batch);
	doForeach<DeleteNodeAction>( m_db, m_keyprefix, key, action);
}

struct BuildMapAction
{
	BuildMapAction( std::map<std::string,std::string>& map_, bool inv_)
		:map(&map_),inv(inv_){}
	BuildMapAction( const BuildMapAction& o)
		:map(o.map),inv(o.inv){}

	void operator()( const leveldb::Slice& keyslice, const leveldb::Slice& valueslice)
	{
		if (inv)
		{
			(*map)[ valueslice.ToString()] = keyslice.ToString();
		}
		else
		{
			(*map)[ keyslice.ToString()] = valueslice.ToString();
		}
	}

private:
	std::map<std::string,std::string>* map;
	bool inv;
};


std::map<std::string,std::string> KeyValueStorage::getMap( const Key& key)
{
	std::map<std::string,std::string> rt;
	BuildMapAction action( rt, false);
	doForeach( m_db, m_keyprefix, key, action);
	return rt;
}

std::map<std::string,std::string> KeyValueStorage::getInvMap( const Key& key)
{
	std::map<std::string,std::string> rt;
	BuildMapAction action( rt, true);
	doForeach( m_db, m_keyprefix, key, action);
	return rt;
}

