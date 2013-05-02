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
#include "keytable.hpp"
#include "file.hpp"
#include <kcdbext.h>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

using namespace strus;

struct KeyTable::Impl
{
	Impl( const std::string& type_, const std::string& name_, const std::string& path_, bool writemode_)
		:isopen(false)
		,writemode(writemode_)
		,type(type_)
		,name(name_)
		,path(path_)
		,nof_deleted(0){}
	~Impl()
	{
		if (isopen) close();
	}

	void close();
	bool open();
	static void create( const std::string& type_, const std::string& name_, const std::string& path_);

	void setError( const std::string& msg);

	Index findKey( const std::string& key) const;
	Index insertKey( const std::string& key);
	bool removeKey( const std::string& key);
	std::string getIdentifier( const Index& idx) const;

	bool isopen;
	bool writemode;
	std::string type;
	std::string name;
	std::string path;
	Index nof_deleted;
	kyotocabinet::IndexDB keydb;
	kyotocabinet::IndexDB strdb;
	std::string lasterror;
	int lasterrno;
};

void KeyTable::Impl::setError( const std::string& msg)
{
	std::string keyfilepath( filepath( path, name, type));
	kyotocabinet::BasicDB::Error kcferr = keydb.error();
	lasterrno = (int)kcferr.code();
	if (!lasterrno)
	{
		kcferr = strdb.error();
		lasterrno = (int)kcferr.code();
	}
	if (lasterrno)
	{
		lasterror = msg + " (" + kcferr.message() + "), file '" + keyfilepath + "'";
	}
	else
	{
		lasterror = msg + ", file '" + keyfilepath + "'";
	}
}

void KeyTable::Impl::close()
{
	isopen = false;
	if (isopen)
	{
		keydb.close();
		strdb.close();
	}
}

static const char* g_metakey_nof_deleted = "\001\002#D";

void KeyTable::Impl::create( const std::string& type_, const std::string& name_, const std::string& path_)
{
	kyotocabinet::IndexDB keydb_;
	kyotocabinet::IndexDB strdb_;
	const std::string invtype_ = type_ + "i";
	std::string keyfilepath( filepath( path_, name_ + "." + type_, "kch"));
	std::string strfilepath( filepath( path_, name_ + "." + invtype_, "kch"));

	//create tables:
	if (!keydb_.open( keyfilepath, kyotocabinet::BasicDB::OWRITER | kyotocabinet::BasicDB::OCREATE | kyotocabinet::BasicDB::OTRUNCATE))
	{
		kyotocabinet::BasicDB::Error kcferr = keydb_.error();
		int lasterrno_ = (int)kcferr.code();
		std::string lasterror_;
		if (lasterrno_)
		{
			lasterror_ = std::string("failed to create key table for ") + type_ + " (" + kcferr.message() + ") at '" + keyfilepath + "'";
		}
		else
		{
			lasterror_ = std::string("failed to create key table for ") + type_ + " at '" + keyfilepath + "'";
		}
		throw std::runtime_error( lasterror_);
	}
	if (!strdb_.open( strfilepath, kyotocabinet::BasicDB::OWRITER | kyotocabinet::BasicDB::OCREATE))
	{
		kyotocabinet::BasicDB::Error kcferr = strdb_.error();
		int lasterrno_ = (int)kcferr.code();
		std::string lasterror_;
		if (lasterrno_)
		{
			lasterror_ = std::string("failed to create inverse key table for ") + invtype_ + " (" + kcferr.message() + ") at '" + strfilepath + "'";
		}
		else
		{
			lasterror_ = std::string("failed to create inverse key table for ") + invtype_ + " at '" + strfilepath + "'";
		}
		keydb_.close();
		throw std::runtime_error( lasterror_);
	}
	//write initial meta data:
	Index nof_deleted_ = 0;
	if (!keydb_.set( g_metakey_nof_deleted, std::strlen(g_metakey_nof_deleted), (char*)&nof_deleted_, sizeof(nof_deleted_)))
	{
		kyotocabinet::BasicDB::Error kcferr = keydb_.error();
		int lasterrno_ = (int)kcferr.code();
		std::string lasterror_;
		if (lasterrno_)
		{
			lasterror_ = std::string("failed to insert meta data in created key table for ") + type_ + " (" + kcferr.message() + ") at '" + keyfilepath + "'";
		}
		else
		{
			lasterror_ = std::string("failed to insert meta data in created key table for ") + type_ + " at '" + keyfilepath + "'";
		}
		keydb_.close();
		strdb_.close();
		throw std::runtime_error( lasterror_);
	}
}

bool KeyTable::Impl::open()
{
	//open tables:
	const std::string invtype = type + "i";
	std::string keyfilepath( filepath( path, name + "." + type, "kch"));
	std::string strfilepath( filepath( path, name + "." + invtype, "kch"));

	if (!keydb.open( keyfilepath, writemode?kyotocabinet::BasicDB::OWRITER:kyotocabinet::BasicDB::OREADER))
	{
		setError( "failed to open key table");
		return false;
	}
	if (!strdb.open( keyfilepath, writemode?kyotocabinet::BasicDB::OWRITER:kyotocabinet::BasicDB::OREADER))
	{
		keydb.close();
		setError( "failed to open inverse key table");
		return false;
	}
	//read metadata:
	std::size_t ptrsize;
	char* ptr = keydb.get( g_metakey_nof_deleted, std::strlen(g_metakey_nof_deleted), &ptrsize);
	if (!ptr || ptrsize != sizeof( Index))
	{
		keydb.close();
		strdb.close();
		delete [] ptr;
		throw std::runtime_error( std::string("corrupt key table ") + type);
	}
	std::memcpy( &nof_deleted, ptr, ptrsize);
	delete [] ptr;
	isopen = true;
	return true;
}

Index KeyTable::Impl::findKey( const std::string& key) const
{
	std::size_t ptrsize = 0;
	kyotocabinet::IndexDB* kdb = const_cast<kyotocabinet::IndexDB*>(&keydb);
	char* ptr = kdb->get( key.c_str(), key.size(), &ptrsize);
	if (!ptr) return 0;
	if (ptrsize != sizeof( Index))
	{
		delete [] ptr;
		throw std::runtime_error( std::string("corrupt key table ") + type);
	}
	Index rt;
	std::memcpy( &rt, ptr, ptrsize);
	delete [] ptr;
	return rt;
}

std::string KeyTable::Impl::getIdentifier( const Index& idx) const
{
	std::size_t ptrsize = 0;
	kyotocabinet::IndexDB* kdb = const_cast<kyotocabinet::IndexDB*>(&strdb);
	char* ptr = kdb->get( (char*)&idx, sizeof(idx), &ptrsize);
	if (!ptr) std::runtime_error( "identifier not found in table");
	std::string rt( ptr, ptrsize);
	delete [] ptr;
	return rt;
}

Index KeyTable::Impl::insertKey( const std::string& key)
{
	if (writemode)
	{
		Index idx = keydb.count() + nof_deleted + 1;
		if (!strdb.set( (char*)&idx, sizeof(idx), key.c_str(), key.size()))
		{
			setError( "failed to insert (inv) into key table");
			return 0;
		}
		if (!keydb.set( key.c_str(), key.size(), (char*)&idx, sizeof(idx)))
		{
			setError( "failed to insert into key table");
			return 0;
		}
		return idx;
	}
	else
	{
		setError( "cannot insert into key table. DB not opened for writing");
		return 0;
	}
}

bool KeyTable::Impl::removeKey( const std::string& key)
{
	if (writemode)
	{
		Index idx = findKey( key);
		if (!idx) return false;

		++nof_deleted;
		if (!keydb.set( g_metakey_nof_deleted, std::strlen(g_metakey_nof_deleted), (char*)&nof_deleted, sizeof(nof_deleted)))
		{
			setError( "failed to delete key from key table (write meta data)");
			--nof_deleted;
			return false;
		}
		if (!keydb.remove( key.c_str(), key.size()))
		{
			setError( "failed to delete key from key table (write meta data)");
			return false;
		}
		strdb.remove( (char*)&idx, sizeof(idx));
		return true;
	}
	else
	{
		setError( "cannot delete key from key table. DB not opened for writing");
		return false;
	}
}


KeyTable::KeyTable( const std::string& type, const std::string& name, const std::string& path, bool writemode)
	:m_impl( new Impl( type, name, path, writemode))
{}

bool KeyTable::open()
{
	return m_impl->open();
}

void KeyTable::close()
{
	return m_impl->close();
}

void KeyTable::create( const std::string& type, const std::string& name, const std::string& path)
{
	Impl::create( type, name, path);
}

KeyTable::~KeyTable()
{
	delete m_impl;
}

const std::string& KeyTable::lastError() const
{
	return m_impl->lasterror;
}

int KeyTable::lastErrno() const
{
	return m_impl->lasterrno;
}

Index KeyTable::findKey( const std::string& key) const
{
	return m_impl->findKey( key);
}

std::string KeyTable::getIdentifier( const Index& idx) const
{
	return m_impl->getIdentifier( idx);
}

Index KeyTable::insertKey( const std::string& key)
{
	return m_impl->insertKey( key);
}

bool KeyTable::removeKey( const std::string& key)
{
	return m_impl->removeKey( key);
}


