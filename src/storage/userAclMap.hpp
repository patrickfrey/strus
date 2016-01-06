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
#include "private/localStructAllocator.hpp"
#include "blockKey.hpp"
#include <cstdlib>

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class DatabaseTransactionInterface;

class UserAclMap
{
public:
	explicit UserAclMap( DatabaseClientInterface* database_)
		:m_database(database_){}

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

	void renameNewDocNumbers( const std::map<Index,Index>& renamemap);
	void getWriteBatch( DatabaseTransactionInterface* transaction);

private:
	void markSetElement(
		const Index& userno,
		const Index& docno,
		bool isMember);

	void clear();

public:
	typedef std::pair<Index,Index> MapKey;
	typedef LocalStructAllocator<std::pair<MapKey,bool> > MapAllocator;
	typedef std::less<MapKey> MapCompare;
	typedef std::map<MapKey,bool,MapCompare,MapAllocator> Map;

private:
	DatabaseClientInterface* m_database;
	Map m_usrmap;
	Map m_aclmap;
	std::vector<Index> m_usr_deletes;
	std::vector<Index> m_acl_deletes;
};

}
#endif

