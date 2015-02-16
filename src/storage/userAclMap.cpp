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
#include "userAclMap.hpp"
#include "booleanBlockBatchWrite.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "databaseAdapter.hpp"
#include "keyMap.hpp"

using namespace strus;

void UserAclMap::markSetElement(
	const Index& userno,
	const Index& elemno,
	bool isMember)
{
	Map::iterator mi = m_usrmap.find( MapKey( userno, elemno));
	if (mi == m_usrmap.end())
	{
		m_usrmap[ MapKey( userno, elemno)] = isMember;
	}
	else
	{
		mi->second = isMember;
	}

	mi = m_aclmap.find( MapKey( elemno, userno));
	if (mi == m_aclmap.end())
	{
		m_aclmap[ MapKey( elemno, userno)] = isMember;
	}
	else
	{
		mi->second = isMember;
	}
}

void UserAclMap::defineUserAccess(
	const Index& userno,
	const Index& elemno)
{
	markSetElement( userno, elemno, true);
}

void UserAclMap::deleteUserAccess(
	const Index& userno,
	const Index& elemno)
{
	markSetElement( userno, elemno, false);
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
		throw std::runtime_error("cannot delete user access after defining it for the same user in a transaction");
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
		throw std::runtime_error("cannot define document access after defining it for the same document in a transaction");
	}
}

static void resetAllBooleanBlockElementsFromStorage(
	UserAclMap::Map& map,
	const Index& idx,
	DatabaseAdapter_BooleanBlock_Cursor& dbadapter)
{
	BooleanBlock blk;
	for (bool more=dbadapter.loadFirst( blk)
		;more; more=dbadapter.loadNext( blk))
	{
		char const* blkitr = blk.charptr();
		Index from_;
		Index to_;
		while (blk.getNextRange( blkitr, from_, to_))
		{
			for (; from_<=to_; ++from_)
			{
				map[ UserAclMap::MapKey( blk.id(), from_)]; // reset to false, only if it does not exist
			}
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
		DatabaseAdapter_UserAclBlock_Cursor dbadapter_userAcl( m_database, *di);
		resetAllBooleanBlockElementsFromStorage( m_usrmap, *di, dbadapter_userAcl);
	}

	std::vector<Index>::const_iterator ai = m_acl_deletes.begin(), ae = m_acl_deletes.end();
	for (; ai != ae; ++ai)
	{
		DatabaseAdapter_AclBlock_Cursor dbadapter_acl( m_database, *ai);
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
		DatabaseAdapter_UserAclBlock_Cursor dbadapter_userAcl( m_database, start->first.first);
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
		DatabaseAdapter_AclBlock_Cursor dbadapter_acl( m_database, start->first.first);
		BooleanBlock newblk;

		// [1] Merge new elements with existing upper bound blocks:
		BooleanBlockBatchWrite::mergeNewElements( &dbadapter_acl, ri, re, newblk, transaction);

		// [2] Write the new blocks that could not be merged into existing ones:
		BooleanBlockBatchWrite::insertNewElements( &dbadapter_acl, ri, re, newblk, lastInsertBlockId, transaction);
	}
	m_usrmap.clear();
	m_aclmap.clear();
}


