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

struct UsrAclKey
{
	Index usrno;
	Index docno;

	UsrAclKey( const Index& usrno_, const Index& docno_)
		:usrno(usrno_),docno(docno_){}
	UsrAclKey( const UsrAclKey& o)
		:usrno(o.usrno),docno(o.docno){}
};

struct UsrAclKey_cmpByUsrDoc {
	bool operator()(const UsrAclKey& a, const UsrAclKey& b) const
	{
		return a.usrno == b.usrno ? a.docno < b.docno : a.usrno < b.usrno;
	}
};
struct UsrAclKey_cmpByDocUsr {
	bool operator()(const UsrAclKey& a, const UsrAclKey& b) const
	{
		return a.docno == b.docno ? a.usrno < b.usrno : a.docno < b.docno;
	}
};


class UserAclMap
{
public:
	explicit UserAclMap( DatabaseClientInterface* database_)
		:m_database(database_),m_usrdocmap(),m_docusrmap(),m_usr_deletes(),m_doc_deletes(){}

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

	void clear();

private:
	void markSetElement(
		const Index& userno,
		const Index& docno,
		bool isMember);

public:
	typedef LocalStructAllocator<std::pair<UsrAclKey,bool> > MapAllocator;
	typedef std::map<UsrAclKey,bool,UsrAclKey_cmpByUsrDoc,MapAllocator> UsrDocMap;
	typedef std::map<UsrAclKey,bool,UsrAclKey_cmpByDocUsr,MapAllocator> DocUsrMap;

private:
	DatabaseClientInterface* m_database;
	UsrDocMap m_usrdocmap;
	DocUsrMap m_docusrmap;
	std::vector<Index> m_usr_deletes;
	std::vector<Index> m_doc_deletes;
};

}
#endif

