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
	{
		UsrDocMap::iterator ui = m_usrdocmap.begin(), ue = m_usrdocmap.end();
		while (ui != ue)
		{
			if (KeyMap::isUnknown( ui->first.docno))
			{
				std::map<Index,Index>::const_iterator ri = renamemap.find( ui->first.docno);
				if (ri == renamemap.end())
				{
					throw strus::runtime_error( _TXT( "docno undefined (%s)"), "user doc map");
				}
				Index newdocno = ri->second;
				m_usrdocmap[ UsrAclKey( ui->first.usrno, newdocno)] = ui->second;
				m_usrdocmap.erase( ui++);
			}
			else
			{
				++ui;
			}
		}
	}{
		DocUsrMap::iterator ai = m_docusrmap.begin(), ae = m_docusrmap.end();
		while (ai != ae)
		{
			if (KeyMap::isUnknown( ai->first.docno))
			{
				std::map<Index,Index>::const_iterator ri = renamemap.find( ai->first.docno);
				if (ri == renamemap.end())
				{
					throw strus::runtime_error( _TXT( "docno undefined (%s)"), "doc user map");
				}
				Index newdocno = ri->second;
				m_docusrmap[ UsrAclKey( ai->first.usrno, newdocno)] = ai->second;
				m_docusrmap.erase( ai++);
			}
			else
			{
				++ai;
			}
		}
	}{
		std::vector<Index> newdeletes;
		std::vector<Index>::const_iterator di = m_doc_deletes.begin(), de = m_doc_deletes.end();
		for (; di != de; ++di)
		{
			if (KeyMap::isUnknown( *di))
			{
				std::map<Index,Index>::const_iterator ri = renamemap.find( *di);
				if (ri == renamemap.end())
				{
					throw strus::runtime_error( _TXT( "docno undefined (%s)"), "doc user map");
				}
				newdeletes.push_back( ri->second);
			}
			else
			{
				newdeletes.push_back( *di);
			}
		}
		m_doc_deletes = newdeletes;
	}
}

void UserAclMap::markSetElement(
	const Index& userno,
	const Index& docno,
	bool isMember)
{
	UsrAclKey mkey( userno, docno);
	m_usrdocmap[ mkey] = isMember;
	m_docusrmap[ mkey] = isMember;
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
	UsrDocMap::const_iterator mi = m_usrdocmap.upper_bound( UsrAclKey( userno, 0));
	if (mi == m_usrdocmap.end() || mi->first.usrno == userno)
	{
		m_usr_deletes.push_back( userno);
	}
	else
	{
		throw strus::runtime_error( "%s",  _TXT( "cannot delete user access after defining it for the same user in a transaction"));
	}
}

void UserAclMap::deleteDocumentAccess(
	const Index& docno)
{
	DocUsrMap::const_iterator mi = m_docusrmap.upper_bound( UsrAclKey( 0, docno));
	if (mi == m_docusrmap.end() || mi->first.docno == docno)
	{
		m_doc_deletes.push_back( docno);
	}
	else
	{
		throw strus::runtime_error( "%s",  _TXT( "cannot define document access after defining it for the same document in a transaction"));
	}
}

static void resetAllBooleanBlockElementsFromStorage(
	UserAclMap::UsrDocMap& map,
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
			map[ UsrAclKey( blk.id()/*userno*/, docno)]; // reset to false, only if it does not exist
		}
	}
}

static void resetAllBooleanBlockElementsFromStorage(
	UserAclMap::DocUsrMap& map,
	DatabaseAdapter_BooleanBlock::Cursor& dbadapter)
{
	BooleanBlock blk;
	for (bool more=dbadapter.loadFirst( blk)
		;more; more=dbadapter.loadNext( blk))
	{
		BooleanBlock::NodeCursor cursor;
		Index usrno = blk.getFirst( cursor);
		for (;usrno; usrno = blk.getNext( cursor))
		{
			map[ UsrAclKey( usrno, blk.id()/*docno*/)]; // reset to false, only if it does not exist
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
		DatabaseAdapter_UserAclBlock::Cursor dbadapter_userAcl( m_database, *di, false/*use cache*/);
		resetAllBooleanBlockElementsFromStorage( m_usrdocmap, dbadapter_userAcl);
	}

	std::vector<Index>::const_iterator ai = m_doc_deletes.begin(), ae = m_doc_deletes.end();
	for (; ai != ae; ++ai)
	{
		DatabaseAdapter_AclBlock::Cursor dbadapter_acl( m_database, *ai, false/*use cache*/);
		resetAllBooleanBlockElementsFromStorage( m_docusrmap, dbadapter_acl);
	}
	{
		UsrDocMap::const_iterator mi = m_usrdocmap.begin(), me = m_usrdocmap.end();
		while (mi != me)
		{
			std::vector<BooleanBlock::MergeRange> rangear;
			UsrDocMap::const_iterator start = mi;
	
			defineRangeElement( rangear, mi->first.docno, mi->second);
			for (++mi; mi != me && mi->first.usrno == start->first.usrno; ++mi)
			{
				defineRangeElement( rangear, mi->first.docno, mi->second);
			}
			Index lastInsertBlockId = rangear.back().to;
	
			std::vector<BooleanBlock::MergeRange>::iterator ri = rangear.begin(), re = rangear.end();
			DatabaseAdapter_UserAclBlock::WriteCursor dbadapter_userAcl( m_database, start->first.usrno);
			BooleanBlock newblk;
	
			// [1] Merge new elements with existing upper bound blocks:
			BooleanBlockBatchWrite::mergeNewElements( &dbadapter_userAcl, ri, re, newblk, transaction);
	
			// [2] Write the new blocks that could not be merged into existing ones:
			BooleanBlockBatchWrite::insertNewElements( &dbadapter_userAcl, ri, re, newblk, lastInsertBlockId, transaction);
		}
	}
	{
		DocUsrMap::const_iterator mi = m_docusrmap.begin(), me = m_docusrmap.end();
		while (mi != me)
		{
			std::vector<BooleanBlock::MergeRange> rangear;
			DocUsrMap::const_iterator start = mi;

			defineRangeElement( rangear, mi->first.usrno, mi->second);
			for (++mi; mi != me && mi->first.docno == start->first.docno; ++mi)
			{
				defineRangeElement( rangear, mi->first.usrno, mi->second);
			}
			Index lastInsertBlockId = rangear.back().to;

			std::vector<BooleanBlock::MergeRange>::iterator ri = rangear.begin(), re = rangear.end();
			DatabaseAdapter_AclBlock::WriteCursor dbadapter_acl( m_database, start->first.docno);
			BooleanBlock newblk;

			// [1] Merge new elements with existing upper bound blocks:
			BooleanBlockBatchWrite::mergeNewElements( &dbadapter_acl, ri, re, newblk, transaction);

			// [2] Write the new blocks that could not be merged into existing ones:
			BooleanBlockBatchWrite::insertNewElements( &dbadapter_acl, ri, re, newblk, lastInsertBlockId, transaction);
		}
	}
}

void UserAclMap::clear()
{
	m_usrdocmap.clear();
	m_docusrmap.clear();
	m_usr_deletes.clear();
	m_doc_deletes.clear();
}


