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
#include "docnoBlockReader.hpp"
#include <leveldb/write_batch.h>

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

void DocnoBlockMap::deleteDocnoPosting(
	const Index& termtype,
	const Index& termvalue,
	const Index& docno)
{
	defineDocnoPosting( termtype, termvalue, docno, 0, 0.0);
}

void DocnoBlockMap::writeBlock(
	leveldb::WriteBatch& batch,
	const Index& typeno,
	const Index& valueno,
	const DocnoBlock::Element* blkptr,
	std::size_t blksize)
{
	Index docno = blkptr[ blksize-1].docno();

	DatabaseKey key( (char)DatabaseKey::DocnoBlockPrefix,
			 typeno, valueno, docno);
	
	leveldb::Slice keyslice( key.ptr(), key.size());
	leveldb::Slice valueslice(
		(const char*)blkptr, blksize*sizeof(*blkptr));

	batch.Put( keyslice, valueslice);
}

void DocnoBlockMap::deleteBlock(
	leveldb::WriteBatch& batch,
	const Index& typeno,
	const Index& valueno,
	const Index& docno)
{
	DatabaseKey key( (char)DatabaseKey::DocnoBlockPrefix,
			 typeno, valueno, docno);
	
	leveldb::Slice keyslice( key.ptr(), key.size());
	batch.Delete( keyslice);
}

void DocnoBlockMap::writeMergeBlock(
		leveldb::WriteBatch& batch,
		const Index& typeno,
		const Index& valueno,
		ElementMap::const_iterator& ei,
		const ElementMap::const_iterator& ee,
		const DocnoBlock* blk)
{
	// Create the new block to replace the old:
	std::size_t newblksize = 0;
	DocnoBlock::Element newblk[ BlockSize*2];

	// Merge the old block with the found entries
	// into the new block:
	const DocnoBlock::Element* di = blk->data();
	const DocnoBlock::Element* de = blk->data() + blk->size();

	while (ei != ee && ei->first <= blk->back().docno() && di != de)
	{
		if (di->docno() == ei->first)
		{
			if (ei->second.ff())
			{
				//... overwrite old with new definition
				newblk[ newblksize++] = ei->second;
			}
			else
			{
				//... ff==0 means delete old definition
			}
			++di;
			++ei;
		}
		else if (di->docno() < ei->first)
		{
			newblk[ newblksize++] = *di;
			++di;
		}
		else
		{
			if (ei->second.ff())
			{
				newblk[ newblksize++] = ei->second;
			}
			++ei;
		}
		if (newblksize == BlockSize*2)
		{
			writeBlock( batch, typeno, valueno, newblk, BlockSize);
			std::memmove( newblk, newblk + BlockSize,
					BlockSize * sizeof(DocnoBlock::Element));
			newblksize = BlockSize;
		}
	}
	for (; di != de; ++di)
	{
		newblk[ newblksize++] = *di;
		if (newblksize == BlockSize*2)
		{
			writeBlock( batch, typeno, valueno, newblk, BlockSize);
			std::memmove( newblk, newblk + BlockSize,
					BlockSize * sizeof(DocnoBlock::Element));
			newblksize = BlockSize;
		}
	}
	// Write the block to the batch:
	writeBlock( batch, typeno, valueno, newblk, newblksize);
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
			writeMergeBlock(
				batch, mi->first.type, mi->first.value,
				ei, ee, blk);	
		}
		std::size_t newblksize = 0;
		DocnoBlock::Element newblk[ BlockSize];

		// Join new elements with last block to a block with size BlockSize:
		blk = blkreader.readLastBlock();
		if (blk)
		{
			if (blk->size() < BlockSize)
			{
				deleteBlock(
					batch, mi->first.type,
					mi->first.value, blk->back().docno());
				while (newblksize < blk->size())
				{
					newblk[ newblksize] = blk->data()[ newblksize];
					++newblksize;
				}
			}
		}
		// Write rest elements to new blocks:
		while (ei != ee)
		{
			for (;newblksize < BlockSize && ei != ee; ++ei,++newblksize)
			{
				newblk[ newblksize] = ei->second;
			}
			writeBlock( batch, mi->first.type, mi->first.value,
					newblk, newblksize);
			newblksize = 0;
		}
		if (newblksize)
		{
			writeBlock( batch, mi->first.type, mi->first.value,
					newblk, newblksize);
		}
	}

	leveldb::Status status = m_db->Write( leveldb::WriteOptions(), &batch);
	if (!status.ok())
	{
		throw std::runtime_error( status.ToString());
	}
	batch.Clear();
	m_map.clear();
}


