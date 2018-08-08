/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_DOCUMENT_BLOCK_ITERATOR_TEMPLATE_HPP_INCLUDED
#define _STRUS_DOCUMENT_BLOCK_ITERATOR_TEMPLATE_HPP_INCLUDED
#include "strus/reference.hpp"
#include "databaseAdapter.hpp"
#include "docIndexNode.hpp"

namespace strus {

/// \brief Forward declaration
class DatabaseClientInterface;
/// \brief Forward declaration
class StorageClient;

/// \brief Helper template for base implementation of iterators on structure blocks like posinfo or posinfo range structures
template <class DatabaseAdapterType, class BlockType>
class DocumentBlockIteratorTemplate
{
public:
	DocumentBlockIteratorTemplate( const DatabaseAdapterType& dbadapter_)
		:m_dbadapter(dbadapter_),m_blk(),m_docno(0),m_docno_start(0),m_docno_end(0){}
	~DocumentBlockIteratorTemplate(){}

	Index skipDoc( const Index& docno_)
	{
		if (m_blk.empty())
		{
			// [A] No block loaded yet
			if (m_dbadapter.loadUpperBound( docno_, m_blk))
			{
				m_docno_start = m_blk.firstDoc( m_blkCursor);
				m_docno_end = m_blk.id();
				return m_docno = m_blk.skipDoc( docno_, m_blkCursor);
			}
			else
			{
				m_blkCursor.reset();
				return m_docno = m_docno_start = m_docno_end = 0;
			}
		}
		else
		{
			if (m_docno_start <= docno_ && m_docno_end >= docno_)
			{
				// [B] Document postings are in the same block as for the last query
				return m_docno = m_blk.skipDoc( docno_, m_blkCursor);
			}
			else if (docno_ > m_docno_end && m_docno_end + (m_docno_end - m_docno_start) > docno_)
			{
				// [C] Try to get document postings from a follow block
				while (m_dbadapter.loadNext( m_blk))
				{
					m_docno_start = m_blk.firstDoc( m_blkCursor);
					m_docno_end = m_blk.id();
	
					if (docno_ >= m_docno_start && docno_ <= m_docno_end)
					{
						return m_docno = m_blk.skipDoc( docno_, m_blkCursor);
					}
					else if (!m_blk.isFollowBlockAddress( docno_))
					{
						if (m_dbadapter.loadUpperBound( docno_, m_blk))
						{
							m_docno_start = m_blk.firstDoc( m_blkCursor);
							m_docno_end = m_blk.id();
							return m_docno = m_blk.skipDoc( docno_, m_blkCursor);
						}
						else
						{
							m_blkCursor.reset();
							return m_docno = m_docno_start = m_docno_end = 0;
						}
					}
				}
				m_blkCursor.reset();
				return m_docno = m_docno_start = m_docno_end = 0;
			}
			else
			{
				// [D] Document postings are in a 'far away' block
				if (m_dbadapter.loadUpperBound( docno_, m_blk))
				{
					m_docno_start = m_blk.firstDoc( m_blkCursor);
					m_docno_end = m_blk.id();
					return m_docno = m_blk.skipDoc( docno_, m_blkCursor);
				}
				else
				{
					m_blkCursor.reset();
					return m_docno = m_docno_start = m_docno_end = 0;
				}
			}
		}
	}

	Index docno() const					{return m_docno;}
	Index docno_start() const				{return m_docno_start;}
	Index docno_end() const					{return m_docno_end;}

	bool isCloseCandidate( const Index& docno_) const	{return m_docno_start <= docno_ && m_docno_end >= docno_;}
	const BlockType& currentBlock() const			{return m_blk;}
	DocIndexNodeCursor& currentBlockCursor()		{return m_blkCursor;}
	const DocIndexNodeCursor& currentBlockCursor() const	{return m_blkCursor;}

private:
	DatabaseAdapterType m_dbadapter;
	BlockType m_blk;
	DocIndexNodeCursor m_blkCursor;
	Index m_docno;
	Index m_docno_start;
	Index m_docno_end;
};

}
#endif

