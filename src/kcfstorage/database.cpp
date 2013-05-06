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
#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <boost/lexical_cast.hpp>

using namespace strus;

enum
{
	SmallBlockSize=128,
	IndexBlockSize=4096
};

void StorageDB::create( const std::string& name_, const std::string& path_)
{
	KeyTable::create( "tetab", name_, path_);
	KeyTable::create( "tytab", name_, path_);
	KeyTable::create( "dctab", name_, path_);
	PersistentList<DocNumber>::create( "rdlst", name_, path_);
	BlockTable::create( "smblk", SmallBlockSize, name_, path_);
	BlockTable::create( "ixblk", IndexBlockSize, name_, path_);
}

StorageDB::StorageDB( const std::string& name_, const std::string& path_, bool writemode_)
	:m_name(name_)
	,m_path(path_)
	,m_termtable("tetab",name_,path_,writemode_)
	,m_typetable("tytab",name_,path_,writemode_)
	,m_docidtable("dctab",name_,path_,writemode_)
	,m_deldocidlist("rdlst",name_,path_,writemode_)
	,m_smallblktable("smblk",SmallBlockSize,name_,path_,writemode_)
	,m_indexblktable("ixblk",IndexBlockSize,name_,path_,writemode_)
{}

void StorageDB::close()
{
	m_termtable.close();
	m_typetable.close();
	m_docidtable.close();
	m_deldocidlist.close();
	m_smallblktable.close();
	m_indexblktable.close();
}

void StorageDB::open()
{
	m_termtable.open();
	m_typetable.open();
	m_docidtable.open();
	m_deldocidlist.open();
	m_smallblktable.open();
	m_indexblktable.open();
}

StorageDB::~StorageDB()
{
	close();
}

static void packIndex( std::string& buf, Index idx)
{
	enum {MaxChr=32};
	unsigned char chrbuf[ MaxChr];
	unsigned char chridx = MaxChr;
	if (idx < 127)
	{
		buf.push_back( idx);
	}
	else
	{
		chrbuf[ chridx-1] = (unsigned char)((idx & 0x7F));
		idx >>= 7;
		while (idx > 0 && --chridx > 0)
		{
			chrbuf[ chridx-1] = (unsigned char)((idx & 0x7F)
					| 0x80);
			idx >>= 7;
		}
		buf.append( (char*)chrbuf+chridx, MaxChr-chridx);
	}
}

static Index unpackIndex( std::string::const_iterator& itr, const std::string::const_iterator& end)
{
	Index rt = 0;
	while (itr != end)
	{
		rt = (rt << 7) + (*itr & 0x7F);
		if ((*itr & 0x80) == 0) break;
	}
	return rt;
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

DocNumber StorageDB::findDocumentNumber( const std::string& docid) const
{
	DocNumber rt = m_docidtable.findKey( docid);
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




