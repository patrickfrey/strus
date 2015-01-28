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
#ifndef _STRUS_FORWARD_INDEX_ITERATOR_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_ITERATOR_HPP_INCLUDED
#include "strus/forwardIteratorInterface.hpp"
#include "databaseKey.hpp"
#include "storage.hpp"
#include "blockStorage.hpp"
#include "forwardIndexBlock.hpp"
#include <string>
#include <leveldb/db.h>

namespace strus
{

/// \brief Forward declaration
class DatabaseInterface;

/// \brief Forward index for the index based on LevelDB
class ForwardIterator
	:public ForwardIteratorInterface
{
public:
	ForwardIterator(
		const Storage* storage_,
		DatabaseInterface* database_,
		const std::string& type_);

	virtual ~ForwardIterator();

	/// \brief Define the document of the items inspected
	virtual void skipDoc( const Index& docno_);

	/// \brief Return the next matching position higher than or equal to firstpos in the current document.
	virtual Index skipPos( const Index& firstpos_);

	/// \brief Fetch the item at the current position
	virtual std::string fetch();

private:
	DatabaseInterface* m_database;
	BlockStorage<ForwardIndexBlock>* m_forwardBlockStorage;
	const ForwardIndexBlock* m_curblock;
	Index m_curblock_firstpos;
	Index m_curblock_lastpos;
	char const* m_blockitr;
	Index m_docno;
	Index m_typeno;
	Index m_curpos;
};

}//namespace
#endif


