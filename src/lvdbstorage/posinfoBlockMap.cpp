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
#include "keyMap.hpp"

using namespace strus;

void PosinfoBlockMap::definePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	const std::vector<Index>& pos)
{
	BlockKey dbkey( termtype, termvalue);

	Map::iterator mi = m_map.find( dbkey.index());
	if (mi == m_map.end())
	{
		m_map[ dbkey.index()].define( docno, pos);
	}
	else
	{
		mi->second.define( docno, pos);
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
		BlockKey dbkey( mi->first);

		if (KeyMap::isUnknown( dbkey.elem(2)))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( dbkey.elem(2));
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (posinfo map)");
			}
			BlockKey newkey( dbkey.elem(1), ri->second);

			PosinfoBlockElementMap& newelem = m_map[ newkey.index()];
			newelem.swap( mi->second);
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
		PosinfoBlockElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		if (ei == ee) continue;
		Index lastInsertBlockId = mi->second.lastInsertBlockId();

		BlockStorage<PosinfoBlock> blkstorage(
				m_db, DatabaseKey::PosinfoBlockPrefix,
				BlockKey(mi->first), false);
		PosinfoBlock newposblk;
		std::vector<BooleanBlock::MergeRange> docrangear;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewPosElements( blkstorage, ei, ee, newposblk, docrangear, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewPosElements( blkstorage, ei, ee, newposblk, lastInsertBlockId, docrangear, batch);

		BlockStorage<BooleanBlock> docnostorage(
				m_db, DatabaseKey::DocListBlockPrefix,
				BlockKey(mi->first), false);
		BooleanBlock newdocblk( DatabaseKey::DocListBlockPrefix);

		std::vector<BooleanBlock::MergeRange>::iterator
			di = docrangear.begin(),
			de = docrangear.end();
		lastInsertBlockId = docrangear.back().to;

		// [3] Merge new docno boolean block elements
		mergeNewDocElements( docnostorage, di, de, newdocblk, batch);

		// [4] Merge new docno boolean block elements
		insertNewDocElements( docnostorage, di, de, newdocblk, lastInsertBlockId, batch);
	}
	m_map.clear();
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
		PosinfoBlockElementMap::const_iterator& ei,
		const PosinfoBlockElementMap::const_iterator& ee,
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
		defineDocnoRangeElement( docrangear, ei->docno(), (*ei->ptr())?true:false);

		// Define posinfo block elements (PosinfoBlock):
		if (newposblk.full())
		{
			newposblk.setId( blkid);
			blkstorage.store( newposblk, batch);
			newposblk.clear();
			newposblk.setId( lastInsertBlockId);
		}
		newposblk.append( ei->docno(), ei->ptr());
		blkid = ei->docno();
	}
	if (!newposblk.empty())
	{
		newposblk.setId( blkid);
		blkstorage.store( newposblk, batch);
	}
}

void PosinfoBlockMap::mergeNewPosElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		PosinfoBlockElementMap::const_iterator& ei,
		const PosinfoBlockElementMap::const_iterator& ee,
		PosinfoBlock& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear,
		leveldb::WriteBatch& batch)
{
	const PosinfoBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->docno())))
	{
		// Merge posinfo block elements (PosinfoBlock):
		PosinfoBlockElementMap::const_iterator newposblk_start = ei;
		for (; ei != ee && ei->docno() <= blk->id(); ++ei)
		{
			// Define docno list block elements (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->docno(), (*ei->ptr())?true:false);
		}

		newposblk = PosinfoBlockElementMap::merge( newposblk_start, ei, *blk);
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
	

void PosinfoBlockMap::insertNewDocElements(
		BlockStorage<BooleanBlock>& blkstorage,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newdocblk,
		const Index& lastInsertBlockId,
		leveldb::WriteBatch& batch)
{
	if (newdocblk.id() < lastInsertBlockId)
	{
		newdocblk.setId( lastInsertBlockId);
	}
	Index blkid = newdocblk.id();
	for (; ei != ee; ++ei)
	{
		// Define posinfo block elements (PosinfoBlock):
		if (newdocblk.full())
		{
			newdocblk.setId( blkid);
			blkstorage.store( newdocblk, batch);
			newdocblk.clear();
			newdocblk.setId( lastInsertBlockId);
		}
		if (ei->isMember)
		{
			newdocblk.defineRange( ei->from, ei->to - ei->from);
		}
		blkid = ei->to;
	}
	if (!newdocblk.empty())
	{
		newdocblk.setId( blkid);
		blkstorage.store( newdocblk, batch);
	}
}

void PosinfoBlockMap::mergeNewDocElements(
		BlockStorage<BooleanBlock>& blkstorage,
		std::vector<BooleanBlock::MergeRange>::iterator& ei,
		const std::vector<BooleanBlock::MergeRange>::iterator& ee,
		BooleanBlock& newdocblk,
		leveldb::WriteBatch& batch)
{
	const BooleanBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->from)))
	{
		// Merge posinfo block elements (PosinfoBlock):
		Index splitStart = 0;
		Index splitEnd = 0;

		std::vector<BooleanBlock::MergeRange>::iterator newdocblk_start = ei;
		for (; ei != ee && ei->from <= blk->id(); ++ei)
		{
			if (ei->to > blk->id())
			{
				// ... last element is overlapping block borders, so we split it
				splitStart = blk->id()+1;
				splitEnd = ei->to; 
				ei->to = blk->id();
			}
		}

		newdocblk = BooleanBlock::merge( newdocblk_start, ei, *blk);
		if (splitStart)
		{
			// ... last element is overlapping block borders, no we assign the second half of it to the block
			--ei;
			ei->from = splitStart;
			ei->to = splitEnd;
		}
		if (blkstorage.loadNext())
		{
			// ... is not the last block, so we store it
			blkstorage.store( newdocblk, batch);
			newdocblk.clear();
		}
		else
		{
			if (newdocblk.full())
			{
				// ... it is the last block, but full
				blkstorage.store( newdocblk, batch);
				newdocblk.clear();
			}
			else
			{
				blkstorage.dispose( newdocblk.id(), batch);
			}
			break;
		}
	}
	if (newdocblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee &&  0!=(blk=blkstorage.loadLast()))
		{
			newdocblk.initcopy( *blk);
			blkstorage.dispose( blk->id(), batch);
		}
	}
}


