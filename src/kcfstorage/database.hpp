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
#include "keytable.hpp"
#include "blktable.hpp"
#include "persistentlist.hpp"
#include <string>
#include <utility>

namespace strus
{

///\class StorageDB
///\brief Implementation of the storage database
class StorageDB
{
public:
	StorageDB( const std::string& name_, const std::string& path_, bool writemode_=false);
	virtual ~StorageDB();

	void open();
	void close();

	static void create( const std::string& name, const std::string& path=std::string());

	TermNumber findTermNumber( const std::string& type, const std::string& value) const;
	DocNumber findDocumentNumber( const std::string& docid) const;

	TermNumber insertTermNumber( const std::string& type, const std::string& value);
	DocNumber insertDocumentNumber( const std::string& docid);

	std::pair<std::string,std::string> getTerm( const TermNumber& tn);
	std::string getDocumentId( const DocNumber& dn);

	const std::string& lastError();
	int lastErrno();

private:
	void clearTransaction();

private:
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

	std::string m_name;
	std::string m_path;
	KeyTable m_termtable;
	KeyTable m_typetable;
	KeyTable m_docidtable;
	PersistentList<DocNumber> m_deldocidlist;
	BlockTable m_smallblktable;
	BlockTable m_indexblktable;
};

} //namespace
#endif


