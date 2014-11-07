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
#ifndef _STRUS_FORWARD_INDEX_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_HPP_INCLUDED
#include "strus/forwardIndexViewerInterface.hpp"
#include "databaseKey.hpp"
#include "storage.hpp"
#include <string>
#include <leveldb/db.h>

namespace strus
{

/// \brief Forward index for the index based on LevelDB
class ForwardIndexViewer
	:public ForwardIndexViewerInterface
{
public:
	ForwardIndexViewer(
		Storage* storage_,
		leveldb::DB* db_,
		const std::string& type_);

	virtual ~ForwardIndexViewer();

	/// \brief Define the document of the items inspected
	virtual void initDoc( const Index& docno_);

	/// \brief Return the next matching position higher than or equal to firstpos in the current document.
	virtual Index skipPos( const Index& firstpos_);

	/// \brief Fetch the item at the current position (defined by initType(const std::string&) and initDoc( const Index&))
	virtual std::string fetch();

private:
	void buildKey( int level);

private:
	Storage* m_storage;
	leveldb::DB* m_db;
	leveldb::Iterator* m_itr;
	std::string m_type;
	Index m_docno;
	Index m_typeno;
	Index m_pos;
	DatabaseKey m_key;
	int m_keylevel;
	std::size_t m_keysize_docno;
	std::size_t m_keysize_typeno;
};

}//namespace
#endif


