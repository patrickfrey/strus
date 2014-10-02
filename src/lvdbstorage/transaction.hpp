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
#ifndef _STRUS_LVDB_STORAGE_TRANSACTION_HPP_INCLUDED
#define _STRUS_LVDB_STORAGE_TRANSACTION_HPP_INCLUDED
#include "storage.hpp"
#include <vector>
#include <leveldb/db.h>

namespace strus {

/// \class Transaction
/// \brief Storage insert or update transaction
class Transaction
	:public StorageInterface::TransactionInterface
{
public:
	Transaction( Storage* storage_, const std::string& docid_);

	virtual ~Transaction();
	virtual void addTermOccurrence(
			const std::string& type_,
			const std::string& id_,
			const Index& position_);
	virtual void commit();

private:
	typedef std::pair<Index,Index> TermMapKey;
	typedef std::map< TermMapKey, std::vector<Index> > TermMap;
	typedef std::map< Index, std::string > InvMap;

private:
	Storage* m_storage;
	std::string m_docid;
	TermMap m_terms;
	InvMap m_invs;
};

}
#endif


