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
#include "posinfoBlockMap.hpp"
#include "booleanBlockMap.hpp"
#include "keyMap.hpp"
#include "indexPacker.hpp"

using namespace strus;

PosinfoBlockMap::PosinfoBlockMap( leveldb::DB* db_)
	:m_db(db_),m_lastkey(0)
{
	m_strings.push_back( '\0');
}

PosinfoBlockMap::PosinfoBlockMap( const PosinfoBlockMap& o)
	:m_db(o.m_db),m_map(o.m_map),m_elements(o.m_elements),m_strings(o.m_strings),m_lastkey(o.m_lastkey)
{}

void PosinfoBlockMap::clear()
{
	m_map.clear();
	m_elements.clear();
	m_strings.clear();
	m_strings.push_back('\0');
	m_lastkey = 0;
}

void PosinfoBlockMap::definePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	const std::vector<Index>& pos)
{
	BlockKeyIndex key = BlockKey( termtype, termvalue).index();
	Map::const_iterator mi = m_map.find( key);
	if (mi == m_map.end())
	{
		if (m_elements.size()) m_elements.push_back( Element( 0, 0));

		m_map[ key] = m_elements.size();
		m_lastkey = key;
	}
	else
	{
		if (m_lastkey != key || (m_elements.size() && docno < m_elements.back().docno))
		{
			throw std::runtime_error( "internal: posinfo postings not added grouped by term in ascending docno order");
		}
	}
	m_elements.push_back( Element( docno, pos.size()?m_strings.size():0));
	if (pos.size())
	{
		std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
		for (; pi != pe; ++pi)
		{
			packIndex( m_strings, *pi);
		}
		m_strings.push_back( '\0');
	}
}

void PosinfoBlockMap::deletePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno)
{
	definePosinfoPosting( termtype, termvalue, docno, std::vector<Index>());
}

void PosinfoBlockMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
{
	typename Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index termno = BlockKey(mi->first).elem(2);
		if (KeyMap::isUnknown( termno))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( termno);
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (posinfo map)");
			}
			BlockKey newkey( BlockKey(mi->first).elem(1), ri->second);

			m_map[ newkey.index()] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}
}

void PosinfoBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	typename Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		std::vector<Element>::const_iterator
			ei = m_elements.begin() + mi->second,
			ee = m_elements.end();
		std::vector<Element>::const_iterator estart = ei;
		for (; ei != ee && ei->docno; ++ei){}

		std::vector<Element>::const_iterator lasti = ei;
		Index lastInsertBlockId = (--lasti)->docno;

		BlockStorage<PosinfoBlock> blkstorage(
				m_db, DatabaseKey::PosinfoBlockPrefix,
				BlockKey( mi->first), false);
		PosinfoBlock newposblk;
		std::vector<BooleanBlock::MergeRange> docrangear;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewPosElements( blkstorage, estart, ei, newposblk, docrangear, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewPosElements( blkstorage, estart, ei, newposblk, lastInsertBlockId, docrangear, batch);

		BlockStorage<BooleanBlock> docnostorage(
				m_db, DatabaseKey::DocListBlockPrefix,
				BlockKey( mi->first), false);
		BooleanBlock newdocblk( DatabaseKey::DocListBlockPrefix);

		std::vector<BooleanBlock::MergeRange>::iterator
			di = docrangear.begin(),
			de = docrangear.end();
		lastInsertBlockId = docrangear.back().to;

		// [3] Merge new docno boolean block elements
		BooleanBlockMap::mergeNewElements( docnostorage, di, de, newdocblk, batch);

		// [4] Merge new docno boolean block elements
		BooleanBlockMap::insertNewElements( docnostorage, di, de, newdocblk, lastInsertBlockId, batch);
	}
	clear();
}

void PosinfoBlockMap::defineDocnoRangeElement(
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

void PosinfoBlockMap::insertNewPosElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		std::vector<Element>::const_iterator& ei,
		const std::vector<Element>::const_iterator& ee,
		PosinfoBlock& newposblk,
		const Index& lastInsertBlockId,
		std::vector<BooleanBlock::MergeRange>& docrangear,
		leveldb::WriteBatch& batch)
{
	if (newposblk.id() < lastInsertBlockId)
	{
		newposblk.setId( lastInsertBlockId);
	}
	Index blkid = newposblk.id();
	for (; ei != ee; ++ei)
	{
		// Define docno list block elements (BooleanBlock):
		defineDocnoRangeElement( docrangear, ei->docno, ei->posinfoidx?true:false);

		// Define posinfo block elements (PosinfoBlock):
		if (newposblk.full())
		{
			newposblk.setId( blkid);
			blkstorage.store( newposblk, batch);
			newposblk.clear();
			newposblk.setId( lastInsertBlockId);
		}
		newposblk.append( ei->docno, m_strings.c_str() + ei->posinfoidx);
		blkid = ei->docno;
	}
	if (!newposblk.empty())
	{
		newposblk.setId( blkid);
		blkstorage.store( newposblk, batch);
	}
}

void PosinfoBlockMap::mergeNewPosElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		std::vector<Element>::const_iterator& ei,
		const std::vector<Element>::const_iterator& ee,
		PosinfoBlock& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear,
		leveldb::WriteBatch& batch)
{
	const PosinfoBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->docno)))
	{
		// Merge posinfo block elements (PosinfoBlock):
		std::vector<Element>::const_iterator newposblk_start = ei;
		for (; ei != ee && ei->docno <= blk->id(); ++ei)
		{
			// Define docno list block elements (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->docno, ei->posinfoidx?true:false);
		}

		newposblk = mergePosBlock( newposblk_start, ei, *blk);
		if (blkstorage.loadNext())
		{
			// ... is not the last block, so we store it
			blkstorage.store( newposblk, batch);
			newposblk.clear();
		}
		else
		{
			if (newposblk.full())
			{
				// ... it is the last block, but full
				blkstorage.store( newposblk, batch);
				newposblk.clear();
			}
			else
			{
				blkstorage.dispose( newposblk.id(), batch);
			}
			break;
		}
	}
	if (newposblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee &&  0!=(blk=blkstorage.loadLast()))
		{
			newposblk.initcopy( *blk);
			blkstorage.dispose( blk->id(), batch);
		}
	}
}

PosinfoBlock PosinfoBlockMap::mergePosBlock( 
		std::vector<Element>::const_iterator ei,
		const std::vector<Element>::const_iterator& ee,
		const PosinfoBlock& oldblk)
{
	PosinfoBlock rt;
	rt.setId( oldblk.id());

	char const* old_blkptr = oldblk.begin();
	Index old_docno = oldblk.docno_at( old_blkptr);

	while (ei != ee && old_docno)
	{
		if (ei->docno <= old_docno)
		{
			if (ei->posinfoidx)
			{
				//... append only if not empty (empty => delete)
				rt.append( ei->docno, m_strings.c_str() + ei->posinfoidx);
			}
			if (ei->docno == old_docno)
			{
				//... defined twice -> prefer new entry and ignore old
				old_blkptr = oldblk.nextDoc( old_blkptr);
				old_docno = oldblk.docno_at( old_blkptr);
			}
			++ei;
		}
		else
		{
			if (!oldblk.empty_at( old_blkptr))
			{
				//... append only if not empty (empty => delete)
				rt.appendPositionsBlock( old_blkptr, oldblk.end_at( old_blkptr));
			}
			old_blkptr = oldblk.nextDoc( old_blkptr);
			old_docno = oldblk.docno_at( old_blkptr);
		}
	}
	while (ei != ee)
	{
		if (ei->posinfoidx)
		{
			//... append only if not empty (empty => delete)
			rt.append( ei->docno, m_strings.c_str() + ei->posinfoidx);
		}
		++ei;
	}
	while (old_docno)
	{
		if (!oldblk.empty_at( old_blkptr))
		{
			//... append only if not empty (empty => delete)
			rt.appendPositionsBlock( old_blkptr, oldblk.end_at( old_blkptr));
		}
		old_blkptr = oldblk.nextDoc( old_blkptr);
		old_docno = oldblk.docno_at( old_blkptr);
	}
	return rt;
}


