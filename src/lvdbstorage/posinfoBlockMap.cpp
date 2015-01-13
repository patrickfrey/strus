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
#include <sstream>
#include <iostream>

using namespace strus;

PosinfoBlockMap::PosinfoBlockMap( leveldb::DB* db_)
	:m_dfmap(db_),m_db(db_),m_docno(0)
{
	m_strings.push_back( '\0');
}

PosinfoBlockMap::PosinfoBlockMap( const PosinfoBlockMap& o)
	:m_dfmap(o.m_dfmap)
	,m_db(o.m_db)
	,m_map(o.m_map)
	,m_strings(o.m_strings)
	,m_invtermmap(o.m_invtermmap)
	,m_invterms(o.m_invterms)
	,m_docno(o.m_docno)
	,m_deletes(o.m_deletes)
{}

void PosinfoBlockMap::clear()
{
	m_map.clear();
	m_strings.clear();
	m_strings.push_back('\0');
	m_invtermmap.clear();
	m_invterms.clear();
	m_docno = 0;
	m_deletes.clear();
}

void PosinfoBlockMap::definePosinfoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno,
	const std::vector<Index>& pos)
{
	if (pos.empty()) return;
	if (termtype == 0 || termvalue == 0)
	{
		std::ostringstream arg;
		arg << '(' << termtype << ',' << termvalue << ',' << docno << ')';
		throw std::runtime_error( std::string("internal: calling definePosinfoPosting with illegal arguments: ") + arg.str());
	}
	MapKey key( termtype, termvalue, docno);
	Map::const_iterator mi = m_map.find( key);

	if (mi == m_map.end() || mi->second == 0)
	{
		if (pos.size())
		{
			m_map[ key] = m_strings.size();

			packIndex( m_strings, pos.size()); //... ff
			std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
			for (; pi != pe; ++pi)
			{
				packIndex( m_strings, *pi);
			}
			m_strings.push_back( '\0');
		}
		else
		{
			m_map[ key] = 0;
		}
	}
	else
	{
		throw std::runtime_error( "document feature defined twice");
	}

	InvTermMap::const_iterator vi = m_invtermmap.find( docno);
	if (vi == m_invtermmap.end())
	{
		if (docno == 0) throw std::runtime_error( "illegal document number for insert (posinfo)");
		if (m_invterms.size()) m_invterms.push_back( InvTerm());

		m_invtermmap[ m_docno = docno] = m_invterms.size();
		m_invterms.push_back( InvTerm( termtype, termvalue, pos.size()));
	}
	else
	{
		if (m_docno != docno)
		{
			throw std::runtime_error( "posinfo not inserted grouped by document (duplicate document inserts in one transaction ?)");
		}
		m_invterms.push_back( InvTerm( termtype, termvalue, pos.size()));
	}
}

void PosinfoBlockMap::deleteIndex( const Index& docno)
{
	InvTermMap::iterator vi = m_invtermmap.find( docno);
	if (vi != m_invtermmap.end())
	{
		InvTermList::const_iterator li = m_invterms.begin() + vi->second, le = m_invterms.end();
		for (; li != le && li->typeno; ++li)
		{
			MapKey key( li->typeno, li->termno, docno);
			Map::iterator mi = m_map.find( key);
			if (mi != m_map.end())
			{
				m_map.erase( mi);
			}
		}
		m_invtermmap.erase( vi);
	}
	m_deletes.push_back( docno);
}

void PosinfoBlockMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
{
	// Rename terms:
	Map::iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Index termno = BlockKey(mi->first.termkey).elem(2);
		if (KeyMap::isUnknown( termno))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( termno);
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (posinfo map)");
			}
			MapKey newkey( BlockKey(mi->first.termkey).elem(1), ri->second, mi->first.docno);
			m_map[ newkey] = mi->second;
			m_map.erase( mi++);
		}
		else
		{
			++mi;
		}
	}

	// Rename inv:
	InvTermList::iterator li = m_invterms.begin(), le = m_invterms.end();
	for (; li != le; ++li)
	{
		if (KeyMap::isUnknown( li->termno))
		{
			std::map<Index,Index>::const_iterator ri = renamemap.find( li->termno);
			if (ri == renamemap.end())
			{
				throw std::runtime_error( "internal: term value undefined (inv posinfo map)");
			}
			li->termno = ri->second;
		}
	}

	// Rename df:
	m_dfmap.renameNewTermNumbers( renamemap);
}

void PosinfoBlockMap::getWriteBatch( leveldb::WriteBatch& batch)
{
	// [1] Get deletes:
	BlockStorage<InvTermBlock> invstorage(
			m_db, DatabaseKey::InverseTermIndex, BlockKey(), false);
	std::vector<Index>::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		const InvTermBlock* invblk = invstorage.load( *di);
		if (invblk)
		{
			char const* ei = invblk->begin();
			const char* ee = invblk->end();
			for (;ei != ee; ei = invblk->next( ei))
			{
				InvTerm it = invblk->element_at( ei);

				MapKey key( it.typeno, it.termno, *di);
				m_map[ key];	//... construct member (default 0) if it does not exist
						// <=> mark as deleted, if not member of set of inserts

				m_dfmap.decrement( it.typeno, it.termno, it.df);
			}
		}
		invstorage.dispose( *di, batch);
	}

	// [2] Get inv and df map inserts:
	InvTermMap::const_iterator vi = m_invtermmap.begin(), ve = m_invtermmap.end();
	for (; vi != ve; ++vi)
	{
		InvTermBlock invblk;
		invblk.setId( vi->first);
		InvTermList::const_iterator li = m_invterms.begin() + vi->second, le = m_invterms.end();

		for (; li != le && li->typeno; ++li)
		{
			// inv blk:
			invblk.append( li->typeno, li->termno, li->df);
			// df map:
			m_dfmap.increment( li->typeno, li->termno, li->df);
		}
		invstorage.store( invblk, batch);
	}

	// [3] Get index inserts and term deletes (defined in [1]):
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		Map::const_iterator
			ei = mi,
			ee = mi;
		for (; ee != me && ee->first.termkey == ei->first.termkey; ++ee){}
		mi = ee;

		Map::const_iterator lasti = ee;
		Index lastInsertBlockId = (--lasti)->first.docno;

		BlockKey blkkey( ei->first.termkey);
		BlockStorage<PosinfoBlock> blkstorage(
				m_db, DatabaseKey::PosinfoBlockPrefix, blkkey, false);
		PosinfoBlock newposblk;
		std::vector<BooleanBlock::MergeRange> docrangear;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewPosElements( blkstorage, ei, ee, newposblk, docrangear, batch);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewPosElements( blkstorage, ei, ee, newposblk, lastInsertBlockId, docrangear, batch);

		BlockStorage<BooleanBlock> docnostorage(
				m_db, DatabaseKey::DocListBlockPrefix, blkkey, false);
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

	// [4] Get df writes:
	m_dfmap.getWriteBatch( batch);

	// [5] Clear the maps:
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
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlock& newposblk,
		const Index& lastInsertBlockId,
		std::vector<BooleanBlock::MergeRange>& docrangear,
		leveldb::WriteBatch& batch)
{
	while (ei != ee || newposblk.size())
	{
		Index blkid = newposblk.id();

		Map::const_iterator bi = ei, be = ei;
		std::size_t blksize = 0;
		for (; be != ee && blksize < PosinfoBlock::MaxBlockSize; ++be)
		{
			blksize += std::strlen( m_strings.c_str() + be->second) + 1;
			blkid = be->first.docno;
		}
		ei = be;

		newposblk.setId( blkid);
		for (; bi != be; ++bi)
		{
			if (bi->second)
			{
				// Define docno list block elements (BooleanBlock):
				defineDocnoRangeElement( docrangear, bi->first.docno, true);

				// Define posinfo block elements (PosinfoBlock):
				newposblk.append( bi->first.docno, m_strings.c_str() + bi->second);
			}
			else
			{
				// Delete docno list block element (BooleanBlock):
				defineDocnoRangeElement( docrangear, bi->first.docno, false);
			}
		}
		blkstorage.store( newposblk, batch);
		newposblk.clear();
	}
}

void PosinfoBlockMap::mergeNewPosElements(
		BlockStorage<PosinfoBlock>& blkstorage,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlock& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear,
		leveldb::WriteBatch& batch)
{
	const PosinfoBlock* blk;
	while (ei != ee && 0!=(blk=blkstorage.load( ei->first.docno)))
	{
		// Merge posinfo block elements (PosinfoBlock):
		Map::const_iterator newposblk_start = ei;
		for (; ei != ee && ei->first.docno <= blk->id(); ++ei)
		{
			// Define docno list block elements (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->first.docno, ei->second?true:false);
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
		Map::const_iterator ei,
		const Map::const_iterator& ee,
		const PosinfoBlock& oldblk)
{
	PosinfoBlock rt;
	rt.setId( oldblk.id());

	char const* old_blkptr = oldblk.begin();
	Index old_docno = oldblk.docno_at( old_blkptr);

	while (ei != ee && old_docno)
	{
		if (ei->first.docno <= old_docno)
		{
			if (ei->second)
			{
				//... append only if not empty (empty => delete)
				rt.append( ei->first.docno, m_strings.c_str() + ei->second);
			}
			if (ei->first.docno == old_docno)
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
		if (ei->second)
		{
			//... append only if not empty (empty => delete)
			rt.append( ei->first.docno, m_strings.c_str() + ei->second);
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

