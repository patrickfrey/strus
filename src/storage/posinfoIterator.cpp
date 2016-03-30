/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "posinfoIterator.hpp"
#include "storageClient.hpp"
#include "private/internationalization.hpp"

using namespace strus;

PosinfoIterator::PosinfoIterator( const StorageClient* storage_, const DatabaseClientInterface* database_, Index termtypeno_, Index termvalueno_)
	:m_storage(storage_)
	,m_dbadapter(database_,termtypeno_,termvalueno_)
	,m_termtypeno(termtypeno_)
	,m_termvalueno(termvalueno_)
	,m_docno(0)
	,m_docno_start(0)
	,m_docno_end(0)
	,m_documentFrequency(-1){}


Index PosinfoIterator::skipDoc( const Index& docno_)
{
	if (m_docno && m_docno == docno_) return m_docno;

	m_positionScanner.clear();
	if (m_posinfoBlk.empty())
	{
		// [A] No block loaded yet
		if (m_dbadapter.loadUpperBound( docno_, m_posinfoBlk))
		{
			m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
			m_docno_end = m_posinfoBlk.id();
			return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
		}
		else
		{
			m_posinfoCursor.reset();
			return m_docno = m_docno_start = m_docno_end = 0;
		}
	}
	else
	{
		if (m_docno_start <= docno_ && m_docno_end >= docno_)
		{
			// [B] Document postings are in the same block as for the last query
			return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
		}
		else if (docno_ > m_docno_end && m_docno_end + (m_docno_end - m_docno_start) > docno_)
		{
			// [C] Try to get document postings from a follow block
			while (m_dbadapter.loadNext( m_posinfoBlk))
			{
				m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
				m_docno_end = m_posinfoBlk.id();

				if (docno_ >= m_docno_start && docno_ <= m_docno_end)
				{
					return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
				}
				else if (!m_posinfoBlk.isFollowBlockAddress( docno_))
				{
					if (m_dbadapter.loadUpperBound( docno_, m_posinfoBlk))
					{
						m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
						m_docno_end = m_posinfoBlk.id();
						return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
					}
					else
					{
						m_posinfoCursor.reset();
						return m_docno = m_docno_start = m_docno_end = 0;
					}
				}
			}
			m_posinfoCursor.reset();
			return m_docno = m_docno_start = m_docno_end = 0;
		}
		else
		{
			// [D] Document postings are in a 'far away' block
			if (m_dbadapter.loadUpperBound( docno_, m_posinfoBlk))
			{
				m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
				m_docno_end = m_posinfoBlk.id();
				return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
			}
			else
			{
				m_posinfoCursor.reset();
				return m_docno = m_docno_start = m_docno_end = 0;
			}
		}
	}
}

Index PosinfoIterator::skipPos( const Index& firstpos_)
{
	if (!m_docno) return 0;
	if (!m_positionScanner.initialized())
	{
		m_positionScanner = m_posinfoBlk.positionScanner_at( m_posinfoCursor);
		if (!m_positionScanner.initialized()) return 0;
	}
	Index rt = m_positionScanner.skip( firstpos_);
	if (rt && rt < firstpos_)
	{
		throw strus::runtime_error( _TXT( "corrupt index (posinfo block)"));
	}
	return rt;
}

unsigned int PosinfoIterator::frequency() const
{
	if (!m_docno) return 0;
	return m_posinfoBlk.frequency_at( m_posinfoCursor);
}

Index PosinfoIterator::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = m_storage->documentFrequency( m_termtypeno, m_termvalueno);
	}
	return m_documentFrequency;
}

