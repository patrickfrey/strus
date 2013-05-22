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

void StorageDB::create( const std::string& name_, const std::string& path_, const Configuration& cfg)
{
	Dictionary::create( "tetab", name_, path_, cfg.expected_nof_terms);
	Dictionary::create( "tytab", name_, path_, cfg.expected_nof_types);
	Dictionary::create( "dctab", name_, path_, cfg.expected_nof_docs);
	PodVector<DocNumber>::create( "rdlst", name_, path_);
	BlockTable::create( "smblk", name_, path_, SmallBlockSize);
	BlockTable::create( "ixblk", name_, path_, IndexBlockSize);
	File::create( filepath( path_, name_, "lock"));
}

StorageDB::StorageDB( const std::string& name_, const std::string& path_)
	:m_name(name_)
	,m_path(path_)
	,m_termtable("tetab",name_,path_)
	,m_termblockmap("telst",name_,path_)
	,m_typetable("tytab",name_,path_)
	,m_docidtable("dctab",name_,path_)
	,m_smallblktable("smblk",name_,path_,SmallBlockSize)
	,m_indexblktable("ixblk",name_,path_,IndexBlockSize)
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

void StorageDB::open( bool writemode_)
{
	m_termtable.open( writemode_);
	m_termblockmap.open( writemode_);
	m_typetable.open( writemode_);
	m_docidtable.open( writemode_);
	m_smallblktable.open( writemode_);
	m_indexblktable.open( writemode_);
	m_transaction_lock.open( writemode_);
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
	Index typeidx = m_typetable.find( type);
	if (!typeidx) return 0;

	packIndex( key, typeidx);
	key.append( value);

	TermNumber rt = m_termtable.find( key);
	return rt;
}

TermNumber StorageDB::insertTermNumber( const std::string& type, const std::string& value)
{
	std::string key;
	Index typeidx = m_typetable.find( type);
	if (!typeidx) typeidx = m_typetable.insert( type);

	packIndex( key, typeidx);
	key.append( value);

	TermNumber rt = m_termtable.insert( key);
	if (m_termblockmap.push_back( 0)+1 != rt)
	{
		throw std::runtime_error( "internal data corruption (term to block map)");
	}
	return rt;
}

std::pair<BlockNumber,bool> StorageDB::getTermBlockNumber( const TermNumber& tn) const
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
	DocNumber rt = m_docidtable.find( docid);
	return rt;
}

DocNumber StorageDB::insertDocumentNumber( const std::string& docid)
{
	DocNumber rt = m_docidtable.insert( docid);
	return rt;
}

std::pair<std::string,std::string> StorageDB::getTerm( const TermNumber& tn) const
{
	std::pair<std::string,std::string> rt;
	std::string key = m_termtable.getIdentifier( tn);
	std::string::const_iterator ki = key.begin(), ke = key.end();
	Index typeidx = unpackIndex( ki, ke);
	rt.first = m_typetable.getIdentifier( typeidx);
	rt.second = std::string( ki, ke);
	return rt;
}

std::string StorageDB::getDocumentId( const DocNumber& dn) const
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

void StorageDB::writeSmallBlock( const BlockNumber& idx, const void* data)
{
	m_smallblktable.writeBlock( idx, data);
}

void StorageDB::writePartialSmallBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	if (start > SmallBlockSize) throw std::logic_error( "parameter out of range");
	m_smallblktable.partialWriteBlock( idx, start, data, SmallBlockSize - start);
}

void StorageDB::writeIndexBlock( const BlockNumber& idx, const void* data)
{
	m_smallblktable.writeBlock( idx, data);
}

void StorageDB::writePartialIndexBlock( const BlockNumber& idx, const void* data, std::size_t start)
{
	if (start > IndexBlockSize) throw std::logic_error( "parameter out of range");
	m_smallblktable.partialWriteBlock( idx, start, data, IndexBlockSize - start);
}

void StorageDB::readSmallBlock( const BlockNumber& idx, void* data) const
{
	m_smallblktable.readBlock( idx, data);
}

void StorageDB::readIndexBlock( const BlockNumber& idx, void* data) const
{
	m_indexblktable.readBlock( idx, data);
}




