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
#ifndef _STRUS_LVDB_USER_ACL_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_LVDB_USER_ACL_BLOCK_MAP_HPP_INCLUDED
#include "strus/index.hpp"
#include "booleanBlock.hpp"
#include "localStructAllocator.hpp"
#include "blockKey.hpp"
#include "blockStorage.hpp"
#include <cstdlib>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>

namespace strus {

class UserAclBlockMap
{
public:
	explicit UserAclBlockMap( leveldb::DB* db_)
		:m_db(db_){}
	UserAclBlockMap( const UserAclBlockMap& o)
		:m_db(o.m_db),m_usrmap(o.m_usrmap),m_aclmap(o.m_aclmap){}

	void defineUserAccess(
		const Index& userno,
		const Index& docno);

	void deleteUserAccess(
		const Index& userno,
		const Index& docno);

	void deleteUserAccess(
		const Index& userno);

	void deleteDocumentAccess(
		const Index& docno);

	void getWriteBatch( leveldb::WriteBatch& batch);

private:
	void markSetElement(
		const Index& userno,
		const Index& docno,
		bool isMember);

public:
	typedef std::pair<Index,Index> MapKey;
	typedef LocalStructAllocator<std::pair<MapKey,bool> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,bool,MapCompare,MapAllocator> Map;

private:
	leveldb::DB* m_db;
	Map m_usrmap;
	Map m_aclmap;
	std::vector<Index> m_usr_deletes;
	std::vector<Index> m_acl_deletes;
};

}
#endif

