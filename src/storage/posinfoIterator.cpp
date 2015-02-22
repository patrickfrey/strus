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
#include "posinfoIterator.hpp"
#include "storage.hpp"
#include "statistics.hpp"

using namespace strus;

PosinfoIterator::PosinfoIterator( const Storage* storage_, DatabaseInterface* database_, Index termtypeno_, Index termvalueno_)
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

				Statistics::increment( Statistics::PosinfoBlockReadBlockFollow);

				if (docno_ >= m_docno_start && docno_ <= m_docno_end)
				{
					return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
				}
				else if (!m_posinfoBlk.isFollowBlockAddress( docno_))
				{
					if (m_dbadapter.loadUpperBound( docno_, m_posinfoBlk))
					{
						Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
						m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
						m_docno_end = m_posinfoBlk.id();
						return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
					}
					else
					{
						Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
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
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
				m_docno_start = m_posinfoBlk.firstDoc( m_posinfoCursor);
				m_docno_end = m_posinfoBlk.id();
				return m_docno = m_posinfoBlk.skipDoc( docno_, m_posinfoCursor);
			}
			else
			{
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
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
		throw std::runtime_error( "internal: corrupt index (posinfo block)");
	}
	return rt;
}

unsigned int PosinfoIterator::frequency()
{
	if (!m_docno) return 0;
	return m_posinfoBlk.frequency_at( m_posinfoCursor);
}

GlobalCounter PosinfoIterator::documentFrequency() const
{
	if (m_documentFrequency < 0)
	{
		m_documentFrequency = m_storage->documentFrequency( m_termtypeno, m_termvalueno);
	}
	return m_documentFrequency;
}

