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

const KeyValueStorage::Value* KeyValueStorage::loadValue( const char* keystr, const std::size_t& keysize)
{
	leveldb::Slice keyslice( keystr, keysize);
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

void KeyValueStorage::storeValue( const char* keystr, const std::size_t& keysize, const Value& value, leveldb::WriteBatch& batch)
{
	leveldb::Slice keyslice( keystr, keysize);
	leveldb::Slice valueslice( value.ptr(), value.size());

	batch.Put( keyslice, valueslice);
}

void KeyValueStorage::storeValueIm( const char* keystr, const std::size_t& keysize, const Value& value)
{
	leveldb::Slice keyslice( keystr, keysize);
	leveldb::Slice valueslice( value.ptr(), value.size());

	leveldb::WriteOptions options;
	options.sync = true;
	leveldb::Status status = m_db->Put( options, keyslice, valueslice);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
}

void KeyValueStorage::disposeValue( const char* keystr, const std::size_t& keysize, leveldb::WriteBatch& batch)
{
	leveldb::Slice keyslice( keystr, keysize);
	batch.Delete( keyslice);
}

const KeyValueStorage::Value* KeyValueStorage::load( const std::string& key)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	return loadValue( keystr.c_str(), keystr.size());
}

const KeyValueStorage::Value* KeyValueStorage::load( const BlockKey& key, const Index& subnode)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	return loadValue( dbkey.ptr(), dbkey.size());
}


void KeyValueStorage::store( const std::string& key, const Value& value, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	storeValue( keystr.c_str(), keystr.size(), value, batch);
}

void KeyValueStorage::store( const BlockKey& key, const Index& subnode, const Value& value, leveldb::WriteBatch& batch)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	return storeValue( dbkey.ptr(), dbkey.size(), value, batch);
}

void KeyValueStorage::store( const BlockKey& key, const Value& value, leveldb::WriteBatch& batch)
{
	DatabaseKey dbkey( m_keyprefix, key);
	return storeValue( dbkey.ptr(), dbkey.size(), value, batch);
}

void KeyValueStorage::storeIm( const std::string& key, const Value& value)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	storeValueIm( keystr.c_str(), keystr.size(), value);
}

void KeyValueStorage::storeIm( const BlockKey& key, const Index& subnode, const Value& value)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	return storeValueIm( dbkey.ptr(), dbkey.size(), value);
}

void KeyValueStorage::storeIm( const BlockKey& key, const Value& value)
{
	DatabaseKey dbkey( m_keyprefix, key);
	return storeValueIm( dbkey.ptr(), dbkey.size(), value);
}

void KeyValueStorage::dispose( const std::string& key, leveldb::WriteBatch& batch)
{
	std::string keystr;
	keystr.push_back( m_keyprefix);
	keystr.append( key);

	disposeValue( keystr.c_str(), keystr.size(), batch);
}

void KeyValueStorage::dispose( const BlockKey& key, const Index& subnode, leveldb::WriteBatch& batch)
{
	DatabaseKey dbkey( m_keyprefix, key, subnode);
	return disposeValue( dbkey.ptr(), dbkey.size(), batch);
}

void KeyValueStorage::dispose( const BlockKey& key, leveldb::WriteBatch& batch)
{
	DatabaseKey dbkey( m_keyprefix, key);
	return disposeValue( dbkey.ptr(), dbkey.size(), batch);
}

template <class Action>
static void doForeach( leveldb::DB* db, const leveldb::Slice& keyslice, Action& action)
{
	leveldb::Iterator* vi = db->NewIterator( leveldb::ReadOptions());
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	for (vi->Seek( keyslice); vi->Valid(); vi->Next())
	{
		if (keyslice.size() > vi->key().size()
		||  0!=std::memcmp( vi->key().data(), keyslice.data(), keyslice.size()))
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

void KeyValueStorage::disposeSubnodes( const BlockKey& key, leveldb::WriteBatch& batch)
{
	DatabaseKey dbkey( m_keyprefix, key);
	leveldb::Slice keyslice( dbkey.ptr(), dbkey.size());

	DeleteNodeAction action( batch);
	doForeach<DeleteNodeAction>( m_db, keyslice, action);
}

struct BuildMapAction
{
	BuildMapAction( std::map<std::string,std::string>& map_, bool inv_)
		:map(&map_),inv(inv_){}
	BuildMapAction( const BuildMapAction& o)
		:map(o.map),inv(o.inv){}

	void operator()( const leveldb::Slice& keyslice, const leveldb::Slice& valueslice)
	{
		std::string keystr( keyslice.data()+1, keyslice.size()-1);
		std::string valuestr( valueslice.data(), valueslice.size());
		if (inv)
		{
			(*map)[ valuestr] = keystr;
		}
		else
		{
			(*map)[ keystr] = valuestr;
		}
	}

private:
	std::map<std::string,std::string>* map;
	bool inv;
};


std::map<std::string,std::string> KeyValueStorage::getMap()
{
	leveldb::Slice keyslice( (const char*)&m_keyprefix, 1);

	std::map<std::string,std::string> rt;
	BuildMapAction action( rt, false);
	doForeach( m_db, keyslice, action);
	return rt;
}

std::map<std::string,std::string> KeyValueStorage::getInvMap()
{
	leveldb::Slice keyslice( (const char*)&m_keyprefix, 1);

	std::map<std::string,std::string> rt;
	BuildMapAction action( rt, true);
	doForeach( m_db, keyslice, action);
	return rt;
}


