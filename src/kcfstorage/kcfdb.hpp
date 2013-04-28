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
#ifndef _STRUS_KCF_STORAGE_DB_HPP_INCLUDED
#define _STRUS_KCF_STORAGE_DB_HPP_INCLUDED
#include "strus/storage.hpp"
#include "strus/position.hpp"

namespace strus
{

///\class StorageDB
///\brief Implementation of the storage database
class StorageDB
{
public:
	StorageDB( const std::string& name, const std::string& path);
	explicit StorageDB( const std::string& name);
	~StorageDB();

	void begin();
	bool commit();
	void rollback();
	std::string lastError();
	int lastErrno();

	static void create( const std::string& name, const std::string& path);
	static void create( const std::string& name);

	TermNumber findTermNumber( const std::string& type, const std::string& value) const;
	DocNumber findDocumentNumber( const std::string& docid) const;

	TermNumber insertTermNumber( const std::string& type, const std::string& value);
	DocNumber insertDocumentNumber( const std::string& docid);

private:
	TermNumber findTermNumber( const std::string& key) const;
	std::string getTermKey( const std::string& type, const std::string& value) const;

private:
	void clearTransaction();

private:
	enum {KCF_ERRORBASE=0x100000, SYS_ERRORBASE=0x000000};

	struct Transaction
	{
		DocNumber docCounter;
		std::map<std::string, DocNumber> docmap;
		std::string errormsg;
		int errorno;

		Transaction()
			:docCounter(0){}
	};
	Transaction m_transaction;

	struct Impl;
	Impl* m_impl;
};

} //namespace
#endif


