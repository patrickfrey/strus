/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "userAclMap.hpp"
#include "booleanBlockBatchWrite.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
#include "keyMap.hpp"

using namespace strus;

void UserAclMap::renameNewDocNumbers( const std::map<Index,Index>& renamemap)
{
	Map::iterator ui = m_usrmap.begin(), ue = m_usrmap.end();
	while (ui != ue)
	{
		Index docno = MapKey(ui->first).second;
		if (KeyMap::isUnknown( docno))
		{
			Index userno = MapKey(ui->first).first;
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "user acl map");
			}
			m_usrmap[ MapKey( userno, ri->second)] = ui->second;
			m_usrmap.erase( ui++);
		}
		else
		{
			++ui;
		}
	}
	Map::iterator ai = m_aclmap.begin(), ae = m_aclmap.end();
	while (ai != ae)
	{
		Index docno = MapKey(ai->first).first;
		if (KeyMap::isUnknown( docno))
		{
			Index userno = MapKey(ai->first).second;
			std::map<Index,Index>::const_iterator ri = renamemap.find( docno);
			if (ri == renamemap.end())
			{
				throw strus::runtime_error( _TXT( "docno undefined (%s)"), "acl user map");
			}
			m_aclmap[ MapKey( ri->second, userno)] = ai->second;
			m_aclmap.erase( ai++);
		}
		else
		{
			++ai;
		}
	}
}

void UserAclMap::markSetElement(
	const Index& userno,
	const Index& docno,
	bool isMember)
{
	Map::iterator mi = m_usrmap.find( MapKey( userno, docno));
	if (mi == m_usrmap.end())
	{
		m_usrmap[ MapKey( userno, docno)] = isMember;
	}
	else
	{
		mi->second = isMember;
	}

	mi = m_aclmap.find( MapKey( docno, userno));
	if (mi == m_aclmap.end())
	{
		m_aclmap[ MapKey( docno, userno)] = isMember;
	}
	else
	{
		mi->second = isMember;
	}
}

void UserAclMap::defineUserAccess(
	const Index& userno,
	const Index& docno)
{
	markSetElement( userno, docno, true);
}

void UserAclMap::deleteUserAccess(
	const Index& userno,
	const Index& docno)
{
	markSetElement( userno, docno, false);
}

void UserAclMap::deleteUserAccess(
	const Index& userno)
{
	Map::iterator mi = m_usrmap.upper_bound( MapKey( userno, 0));
	if (mi == m_usrmap.end() || mi->first.first == userno)
	{
		m_usr_deletes.push_back( userno);
	}
	else
	{
		throw strus::runtime_error( _TXT( "cannot delete user access after defining it for the same user in a transaction"));
	}
}

void UserAclMap::deleteDocumentAccess(
	const Index& docno)
{
	Map::iterator mi = m_aclmap.upper_bound( MapKey( docno, 0));
	if (mi == m_aclmap.end() || mi->first.first == docno)
	{
		m_acl_deletes.push_back( docno);
	}
	else
	{
		throw strus::runtime_error( _TXT( "cannot define document access after defining it for the same document in a transaction"));
	}
}

static void resetAllBooleanBlockElementsFromStorage(
	UserAclMap::Map& map,
	const Index& idx,
	DatabaseAdapter_BooleanBlock::Cursor& dbadapter)
{
	BooleanBlock blk;
	for (bool more=dbadapter.loadFirst( blk)
		;more; more=dbadapter.loadNext( blk))
	{
		BooleanBlock::NodeCursor cursor;
		Index docno = blk.getFirst( cursor);
		for (;docno; docno = blk.getNext( cursor))
		{
			map[ UserAclMap::MapKey( blk.id(), docno)]; // reset to false, only if it does not exist
		}
	}
}

static void defineRangeElement( 
		std::vector<BooleanBlock::MergeRange>& docrangear,
		const Index& docno,
		bool isMember)
{
	if (docrangear.empty())
	{
		docrangear.push_back( BooleanBlock::MergeRange( docno, docno, isMember));
	}
	else
	{
		if (docrangear.back().isMember == isMember && docrangear.back().to+1 == docno)
		{
			docrangear.back().to += 1;
		}
		else
		{
			docrangear.push_back( BooleanBlock::MergeRange( docno, docno, isMember));
		}
	}
}

void UserAclMap::getWriteBatch( DatabaseTransactionInterface* transaction)
{
	std::vector<Index>::const_iterator di = m_usr_deletes.begin(), de = m_usr_deletes.end();
	for (; di != de; ++di)
	{
		DatabaseAdapter_UserAclBlock::Cursor dbadapter_userAcl( m_database, *di, false);
		resetAllBooleanBlockElementsFromStorage( m_usrmap, *di, dbadapter_userAcl);
	}

	std::vector<Index>::const_iterator ai = m_acl_deletes.begin(), ae = m_acl_deletes.end();
	for (; ai != ae; ++ai)
	{
		DatabaseAdapter_AclBlock::Cursor dbadapter_acl( m_database, *ai, false);
		resetAllBooleanBlockElementsFromStorage( m_aclmap, *ai, dbadapter_acl);
	}

	Map::const_iterator mi = m_usrmap.begin(), me = m_usrmap.end();
	while (mi != me)
	{
		std::vector<BooleanBlock::MergeRange> rangear;
		Map::const_iterator start = mi;

		defineRangeElement( rangear, mi->first.second, mi->second);
		for (++mi; mi != me && mi->first.second != start->first.second; ++mi)
		{
			defineRangeElement( rangear, mi->first.second, mi->second);
		}

		Index lastInsertBlockId = rangear.back().to;

		std::vector<BooleanBlock::MergeRange>::iterator ri = rangear.begin(), re = rangear.end();
		DatabaseAdapter_UserAclBlock::WriteCursor dbadapter_userAcl( m_database, start->first.first);
		BooleanBlock newblk;

		// [1] Merge new elements with existing upper bound blocks:
		BooleanBlockBatchWrite::mergeNewElements( &dbadapter_userAcl, ri, re, newblk, transaction);

		// [2] Write the new blocks that could not be merged into existing ones:
		BooleanBlockBatchWrite::insertNewElements( &dbadapter_userAcl, ri, re, newblk, lastInsertBlockId, transaction);
	}

	mi = m_aclmap.begin(), me = m_aclmap.end();
	while (mi != me)
	{
		std::vector<BooleanBlock::MergeRange> rangear;
		Map::const_iterator start = mi;

		defineRangeElement( rangear, mi->first.second, mi->second);
		for (++mi; mi != me && mi->first.second != start->first.second; ++mi)
		{
			defineRangeElement( rangear, mi->first.second, mi->second);
		}
		Index lastInsertBlockId = rangear.back().to;

		std::vector<BooleanBlock::MergeRange>::iterator ri = rangear.begin(), re = rangear.end();
		DatabaseAdapter_AclBlock::WriteCursor dbadapter_acl( m_database, start->first.first);
		BooleanBlock newblk;

		// [1] Merge new elements with existing upper bound blocks:
		BooleanBlockBatchWrite::mergeNewElements( &dbadapter_acl, ri, re, newblk, transaction);

		// [2] Write the new blocks that could not be merged into existing ones:
		BooleanBlockBatchWrite::insertNewElements( &dbadapter_acl, ri, re, newblk, lastInsertBlockId, transaction);
	}
}

void UserAclMap::clear()
{
	m_usrmap.clear();
	m_aclmap.clear();
	m_usr_deletes.clear();
	m_acl_deletes.clear();
}


