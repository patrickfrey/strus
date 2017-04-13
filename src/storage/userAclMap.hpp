/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_STORAGE_USER_ACL_BLOCK_MAP_HPP_INCLUDED
#define _STRUS_STORAGE_USER_ACL_BLOCK_MAP_HPP_INCLUDED
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
	typedef LocalStructAllocator<std::pair<const MapKey,bool> > MapAllocator;
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

