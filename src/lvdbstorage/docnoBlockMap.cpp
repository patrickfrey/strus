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
#include "docnoBlockMap.hpp"
#include "databaseKey.hpp"
#include <boost/scoped_ptr.hpp>

using namespace strus;

void DocnoBlockMap::defineDocnoPosting(
		const Index& termtype,
		const Index& termvalue,
		const Index& docno,
		unsigned int ff,
		float weight)
{
	boost::mutex::scoped_lock( m_mutex);
	Term term( termtype, termvalue);
	DocnoBlock::Element element( docno, ff, weight);
	Map::iterator mi = m_map.find( term);
	if (mi == m_map.end())
	{
		ElementMap em;
		em[ docno] = element;
		m_map[ term] = em;
	}
	else
	{
		mi->second[ docno] = DocnoBlock::Element( docno, ff, weight);
	}
}

void DocnoBlockMap::writeMergeBlock(
		leveldb::WriteBatch& batch,
		ElementMap::const_iterator& ei,
		const ElementMap::const_iterator& ee,
		const DocnoBlock* blk)
{
	Index docno = blk->back().docno();

	ElementMap::const_iterator cnt_ei = ei;
	// Count the entries to be merged into 'blk':
	std::size_t nofmrg = 0;
	for (; cnt_ei != ee; ++cnt_ei)
	{
		if (cnt_ei->first > blk->back().docno()) break;
		++nofmrg;
	}

	// Create the new block to replace the old:
	std::size_t newblksize = 0;
	DocnoBlock::Element* newblk
		= new DocnoBlock::Element[ blk->size() + nofmrg];
	boost::scoped_ptr<DocnoBlock::Element> newblk_ref(newblk);

	// Merge the old block with the found entries
	// into the new block:
	const DocnoBlock::Element*
		di = blk->ar(),
		de = blk->ar() + blk->size();
	for (std::size_t mrgcnt; di != de && mrgcnt < nofmrg;)
	{
		if (di->docno() == ei->docno())
		{
			//... overwrite old with new definition
			newblk[ newblksize++] = *ei;
			++di;
			++ei;
			--mrgcnt;
		}
		else if (di->docno() < ei->docno())
		{
			newblk[ newblksize++] = *di;
			++di;
		}
		else
		{
			newblk[ newblksize++] = *ei;
			++ei;
			--mrgcnt;
		}
	}
	for (; di != de; ++di)
	{
		newblk[ newblksize++] = *di;
	}
	// Write the block to the batch:
	DatabaseKey key(
		(char)DatabaseKey::DocnoBlockPrefix,
		mi->first.type, mi->first.value, docno);
	
	leveldb::Slice keyslice( key.ptr(), key.size());
	leveldb::Slice valueslice(
		(const char*)newblk,
		newblksize*sizeof(*newblk));

	batch.Put( keyslice, valueslice);
}

void DocnoBlockMap::flush()
{
	leveldb::WriteBatch batch;

	boost::mutex::scoped_lock( m_mutex);
	Map::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		ElementMap::const_iterator
			ei = mi->second.begin(),
			ee = mi->second.end();

		DocnoBlockReader blkreader( m_db, mi->first.type, mi->first.value);
		const DocnoBlock* blk;

		while (!!(blk=blkreader.readBlock( ei->first)))
		{
			writeMergeBlock( batch, ei, ee, blk);
		}
		blk = blkreader->readLastBlock();
		if (blk)
		{
			if ()
		}
	}
}


