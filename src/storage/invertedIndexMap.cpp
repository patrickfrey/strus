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
#include "invertedIndexMap.hpp"
#include "booleanBlockBatchWrite.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/databaseTransactionInterface.hpp"
#include "keyMap.hpp"
#include "databaseAdapter.hpp"
#include "indexPacker.hpp"
#include <sstream>
#include <limits>

using namespace strus;

InvertedIndexMap::InvertedIndexMap( DatabaseClientInterface* database_)
	:m_dfmap(database_),m_database(database_),m_docno(0)
{
	m_posinfo.push_back( 0);
}

void InvertedIndexMap::clear()
{
	m_dfmap.clear();
	m_map.clear();
	m_posinfo.clear();
	m_posinfo.push_back( 0);
	m_invtermmap.clear();
	m_invterms.clear();
	m_docno = 0;
	m_deletes.clear();
}

void InvertedIndexMap::definePosinfoPosting(
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
			if (pos.size() > std::numeric_limits<PosinfoBlock::PositionType>::max())
			{
				throw std::runtime_error( "size of document out of range (max 65535)");
			}
			m_map[ key] = m_posinfo.size();

			m_posinfo.push_back( (unsigned short)pos.size());	//... ff
			std::vector<Index>::const_iterator pi = pos.begin(), pe = pos.end();
			for (; pi != pe; ++pi)
			{
				if (*pi > std::numeric_limits<PosinfoBlock::PositionType>::max())
				{
					throw std::runtime_error( "token position out of range (max 65535)");
				}
				m_posinfo.push_back( (unsigned short)*pi);
			}
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
		if (m_docno && m_docno != docno)
		{
			throw std::runtime_error( "inverted index operations not grouped by document");
		}
		m_invterms.push_back( InvTerm( termtype, termvalue, pos.size()));
	}
}

void InvertedIndexMap::deleteIndex( const Index& docno)
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

void InvertedIndexMap::renameNewTermNumbers( const std::map<Index,Index>& renamemap)
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

void InvertedIndexMap::getWriteBatch(
			DatabaseTransactionInterface* transaction,
			StoragePeerTransactionInterface* peerTransaction,
			const KeyMapInv& termTypeMapInv,
			const KeyMapInv& termValueMapInv)
{
	// [1] Get deletes:
	DatabaseAdapter_InverseTerm::ReadWriter dbadapter_inv( m_database);
	std::vector<Index>::const_iterator di = m_deletes.begin(), de = m_deletes.end();
	for (; di != de; ++di)
	{
		InvTermBlock invblk;
		if (dbadapter_inv.load( *di, invblk))
		{
			char const* ei = invblk.begin();
			const char* ee = invblk.end();
			for (;ei != ee; ei = invblk.next( ei))
			{
				InvTerm it = invblk.element_at( ei);

				MapKey key( it.typeno, it.termno, *di);
				m_map[ key];	//... construct member (default 0) if it does not exist
						// <=> mark as deleted, if not member of set of inserts

				m_dfmap.decrement( it.typeno, it.termno, it.ff);
			}
		}
		dbadapter_inv.remove( transaction, *di);
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
			invblk.append( li->typeno, li->termno, li->ff);
			// df map:
			m_dfmap.increment( li->typeno, li->termno, li->ff);
		}
		dbadapter_inv.store( transaction, invblk);
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

		BlockKey blkkey( ei->first.termkey);
		Index typeno = blkkey.elem(1);
		Index termno = blkkey.elem(2);
		DatabaseAdapter_PosinfoBlock::WriteCursor dbadapter_posinfo( m_database, typeno, termno);

		PosinfoBlockBuilder newposblk;
		std::vector<BooleanBlock::MergeRange> docrangear;

		// [1] Merge new elements with existing upper bound blocks:
		mergeNewPosElements( dbadapter_posinfo, transaction, ei, ee, newposblk, docrangear);

		// [2] Write the new blocks that could not be merged into existing ones:
		insertNewPosElements( dbadapter_posinfo, transaction, ei, ee, newposblk, docrangear);

		BooleanBlock newdocblk;

		std::vector<BooleanBlock::MergeRange>::iterator
			di = docrangear.begin(),
			de = docrangear.end();
		Index lastInsertBlockId = docrangear.back().to;

		// [3] Update document list of the term (boolean block) in the database:
		DatabaseAdapter_DocListBlock::WriteCursor dbadapter_doclist( m_database, typeno, termno);

		// [3.1] Merge new docno boolean block elements
		BooleanBlockBatchWrite::mergeNewElements( &dbadapter_doclist, di, de, newdocblk, transaction);

		// [3.2] Insert new docno boolean block elements
		BooleanBlockBatchWrite::insertNewElements( &dbadapter_doclist, di, de, newdocblk, lastInsertBlockId, transaction);
	}

	// [4] Get df writes (and populate df's to other peers, if peerTransaction defined):
	m_dfmap.getWriteBatch( transaction, peerTransaction, termTypeMapInv, termValueMapInv);

	// [5] Clear the maps:
	clear();
}

void InvertedIndexMap::defineDocnoRangeElement(
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

void InvertedIndexMap::insertNewPosElements(
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo, 
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlockBuilder& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
	while (ei != ee)
	{
		if (ei->second)
		{
			// Define posinfo block elements (PosinfoBlock):
			if (newposblk.fitsInto( m_posinfo[ ei->second] + 1) || newposblk.empty())
			{
				// Define docno list block elements (BooleanBlock):
				defineDocnoRangeElement( docrangear, ei->first.docno, true);

				// Define posinfo block elements (PosinfoBlock):
				newposblk.append( ei->first.docno, m_posinfo.data() + ei->second);
				++ei;
			}
			else
			{
				dbadapter_posinfo.store( transaction, newposblk.createBlock());
				newposblk.clear();
			}
		}
		else
		{
			// Delete docno list block element (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->first.docno, false);
		}
	}
	if (!newposblk.empty())
	{
		dbadapter_posinfo.store( transaction, newposblk.createBlock());
		newposblk.clear();
	}
}

void InvertedIndexMap::mergeNewPosElements(
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator& ei,
		const Map::const_iterator& ee,
		PosinfoBlockBuilder& newposblk,
		std::vector<BooleanBlock::MergeRange>& docrangear)
{
	PosinfoBlock blk;
	while (ei != ee && dbadapter_posinfo.loadUpperBound( ei->first.docno, blk))
	{
		// Merge posinfo block elements (PosinfoBlock):
		Map::const_iterator newposblk_start = ei;
		for (; ei != ee && ei->first.docno <= blk.id(); ++ei)
		{
			// Define docno list block elements (BooleanBlock):
			defineDocnoRangeElement( docrangear, ei->first.docno, ei->second?true:false);
		}

		mergePosBlock( dbadapter_posinfo, transaction, newposblk_start, ei, blk, newposblk);
		if (dbadapter_posinfo.loadNext( blk))
		{
			// ... is not the last block, so we store it
			dbadapter_posinfo.store( transaction, newposblk.createBlock());
			newposblk.clear();
		}
		else
		{
			if (newposblk.full())
			{
				// ... it is the last block, but full
				dbadapter_posinfo.store( transaction, newposblk.createBlock());
				newposblk.clear();
			}
			else
			{
				dbadapter_posinfo.remove( transaction, blk.id());
				newposblk.setId(0);
			}
			break;
		}
	}
	if (newposblk.empty())
	{
		// Fill first new block with elements of last 
		// block and dispose the last block:
		if (ei != ee && dbadapter_posinfo.loadLast( blk))
		{
			newposblk = PosinfoBlockBuilder( blk);
			dbadapter_posinfo.remove( transaction, blk.id());
			newposblk.setId(0);
		}
	}
}

void InvertedIndexMap::mergePosBlock( 
		DatabaseAdapter_PosinfoBlock::WriteCursor& dbadapter_posinfo,
		DatabaseTransactionInterface* transaction,
		Map::const_iterator ei,
		const Map::const_iterator& ee,
		const PosinfoBlock& oldblk,
		PosinfoBlockBuilder& newblk)
{
	newblk.setId( oldblk.id());
	PosinfoBlock::Cursor blkcursor;

	Index old_docno = oldblk.firstDoc( blkcursor);
	while (ei != ee && old_docno)
	{
		if (ei->first.docno <= old_docno)
		{
			if (ei->second)
			{
				//... append only if not empty (empty => delete)
				if (!newblk.fitsInto( m_posinfo[ ei->second]))
				{
					newblk.setId(0);
					dbadapter_posinfo.store( transaction, newblk.createBlock());
					newblk.clear();
					newblk.setId( oldblk.id());
				}
				newblk.append( ei->first.docno, m_posinfo.data() + ei->second);
			}
			if (ei->first.docno == old_docno)
			{
				//... defined twice -> prefer new entry and ignore old
				old_docno = oldblk.nextDoc( blkcursor);
			}
			++ei;
		}
		else
		{
			newblk.append( old_docno, oldblk.posinfo_at( blkcursor));
			old_docno = oldblk.nextDoc( blkcursor);
		}
	}
	while (ei != ee)
	{
		if (ei->second)
		{
			//... append only if not empty (empty => delete)
			if (!newblk.fitsInto( m_posinfo[ ei->second]))
			{
				newblk.setId(0);
				dbadapter_posinfo.store( transaction, newblk.createBlock());
				newblk.clear();
				newblk.setId( oldblk.id());
			}
			newblk.append( ei->first.docno, m_posinfo.data() + ei->second);
		}
		++ei;
	}
	while (old_docno)
	{
		if (!newblk.fitsInto( oldblk.frequency_at( blkcursor)))
		{
			newblk.setId(0);
			dbadapter_posinfo.store( transaction, newblk.createBlock());
			newblk.clear();
			newblk.setId( oldblk.id());
		}
		newblk.append( old_docno, oldblk.posinfo_at( blkcursor));
		old_docno = oldblk.nextDoc( blkcursor);
	}
}


