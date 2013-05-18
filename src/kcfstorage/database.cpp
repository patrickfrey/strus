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
#include "database.hpp"
#include "encode.hpp"
#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

using namespace strus;

void StorageDB::create( const std::string& name_, const std::string& path_)
{
	KeyTable::create( "tetab", name_, path_);
	KeyTable::create( "tytab", name_, path_);
	KeyTable::create( "dctab", name_, path_);
	PodVector<DocNumber>::create( "rdlst", name_, path_);
	BlockTable::create( "smblk", SmallBlockSize, name_, path_);
	BlockTable::create( "ixblk", IndexBlockSize, name_, path_);
	File::create( filepath( path_, name_, "lock"));
}

StorageDB::StorageDB( const std::string& name_, const std::string& path_, bool writemode_)
	:m_writemode(writemode_)
	,m_name(name_)
	,m_path(path_)
	,m_termtable("tetab",name_,path_,writemode_)
	,m_termblockmap("telst",name_,path_,writemode_)
	,m_typetable("tytab",name_,path_,writemode_)
	,m_docidtable("dctab",name_,path_,writemode_)
	,m_smallblktable("smblk",SmallBlockSize,name_,path_,writemode_)
	,m_indexblktable("ixblk",IndexBlockSize,name_,path_,writemode_)
	,m_transaction_lock( filepath( path_, name_, "lock"))
{
	m_nulledblock = std::calloc( 1, IndexBlockSize);
	if (!m_nulledblock) throw std::bad_alloc();
}

void StorageDB::close()
{
	m_termtable.close();
	m_termblockmap.close();
	m_typetable.close();
	m_docidtable.close();
	m_smallblktable.close();
	m_indexblktable.close();
	m_transaction_lock.close();
	std::free( m_nulledblock);
}

void StorageDB::open()
{
	m_termtable.open();
	m_termblockmap.open();
	m_typetable.open();
	m_docidtable.open();
	m_smallblktable.open();
	m_indexblktable.open();
	m_transaction_lock.open( m_writemode);
}

void StorageDB::lock()
{
	m_transaction_lock.lock();
}

void StorageDB::unlock()
{
	m_transaction_lock.unlock();
}

StorageDB::~StorageDB()
{
	close();
}

TermNumber StorageDB::findTermNumber( const std::string& type, const std::string& value) const
{
	std::string key;
	Index typeidx = m_typetable.findKey( type);
	if (!typeidx) return 0;

	packIndex( key, typeidx);
	key.append( value);

	TermNumber rt = m_termtable.findKey( key);
	return rt;
}

TermNumber StorageDB::insertTermNumber( const std::string& type, const std::string& value)
{
	std::string key;
	Index typeidx = m_typetable.findKey( type);
	if (!typeidx) typeidx = m_typetable.insertKey( type);

	packIndex( key, typeidx);
	key.append( value);

	TermNumber rt = m_termtable.insertKey( key);
	if (m_termblockmap.push_back( 0)+1 != rt)
	{
		throw std::runtime_error( "internal data corruption (term to block map)");
	}
	return rt;
}

std::pair<BlockNumber,bool> StorageDB::getTermBlockNumber( const TermNumber& tn)
{
	BlockNumber ib = m_termblockmap.get( tn);
	return std::pair<BlockNumber,bool>( ib>>1, ib&1);
}

void StorageDB::setTermBlockNumber( const TermNumber& tn, const BlockNumber& bn, bool isSmallBlock)
{
	m_termblockmap.set( tn, (bn << 1) | (isSmallBlock?1:0));
}

DocNumber StorageDB::findDocumentNumber( const std::string& docid) const
{
	DocNumber rt = m_docidtable.findKey( docid);
	return rt;
}

DocNumber StorageDB::insertDocumentNumber( const std::string& docid)
{
	DocNumber rt = m_docidtable.insertKey( docid);
	return rt;
}

std::pair<std::string,std::string> StorageDB::getTerm( const TermNumber& tn)
{
	std::pair<std::string,std::string> rt;
	std::string key = m_termtable.getIdentifier( tn);
	std::string::const_iterator ki = key.begin(), ke = key.end();
	Index typeidx = unpackIndex( ki, ke);
	rt.first = m_typetable.getIdentifier( typeidx);
	rt.second = std::string( ki, ke);
	return rt;
}

std::string StorageDB::getDocumentId( const DocNumber& dn)
{
	return m_docidtable.getIdentifier( dn);
}

BlockNumber StorageDB::allocSmallBlock()
{
	return m_smallblktable.insertBlock( m_nulledblock);
}

BlockNumber StorageDB::allocIndexBlock()
{
	return m_indexblktable.insertBlock( m_nulledblock);
}

void StorageDB::writeSmallBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	m_smallblktable.writeBlock( idx, data);
}

void StorageDB::writePartialSmallBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	if (start > SmallBlockSize) throw std::logic_error( "parameter out of range");
	m_smallblktable.partialWriteBlock( idx, start, data, SmallBlockSize - start);
}

void StorageDB::writeIndexBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	m_smallblktable.writeBlock( idx, data);
}

void StorageDB::writePartialIndexBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	if (start > IndexBlockSize) throw std::logic_error( "parameter out of range");
	m_smallblktable.partialWriteBlock( idx, start, data, IndexBlockSize - start);
}

void StorageDB::readSmallBlock( const BlockNumber& idx, void* data)
{
	m_smallblktable.readBlock( idx, data);
}

void StorageDB::readIndexBlock( const BlockNumber& idx, void* data)
{
	m_indexblktable.readBlock( idx, data);
}




