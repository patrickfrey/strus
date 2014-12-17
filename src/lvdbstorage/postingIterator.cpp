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
#include "postingIterator.hpp"
#include "storage.hpp"
#include "statistics.hpp"
#include "indexPacker.hpp"
#include "keyValueStorage.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace strus;

#define STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_LOWLEVEL_DEBUG
PostingIterator::PostingIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno, const char* termstr)
#else
PostingIterator::PostingIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno, const char*)
#endif
	:m_db(db_)
	,m_docnoStorage( db_, DatabaseKey::DocnoBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_docnoBlk(0)
	,m_docnoItr(0)
	,m_posinfoStorage( db_, DatabaseKey::PosinfoBlockPrefix, BlockKey( termtypeno, termvalueno), true)
	,m_posinfoBlk(0)
	,m_posinfoItr(0)
	,m_last_docno(0)
	,m_docno(0)
	,m_termtypeno(termtypeno)
	,m_termvalueno(termvalueno)
	,m_documentFrequency(-1)
{
	m_featureid.reserve( 16);
#ifdef STRUS_LOWLEVEL_DEBUG
	m_featureid.append( termstr);
	m_featureid.push_back(':');
	m_featureid.push_back( (char)(termtypeno/10) + '0');
	m_featureid.push_back( (char)(termtypeno%10) + '0');
#else
	packIndex( m_featureid, termtypeno);
	packIndex( m_featureid, termvalueno);
#endif
}

PostingIterator::PostingIterator( const PostingIterator& o)
	:PostingIteratorInterface(o)
	,m_db(o.m_db)
	,m_docnoStorage( o.m_docnoStorage)
	,m_docnoBlk(0)
	,m_docnoItr(0)
	,m_posinfoStorage( o.m_posinfoStorage)
	,m_posinfoBlk(0)
	,m_posinfoItr(0)
	,m_last_docno(0)
	,m_docno(0)
	,m_termtypeno(o.m_termtypeno)
	,m_termvalueno(o.m_termvalueno)
	,m_documentFrequency(o.m_documentFrequency)
	,m_featureid(o.m_featureid)
{}

Index PostingIterator::skipDocDocnoBlock( const Index& docno_)
{
	if (!m_docnoBlk)
	{
		// [A] No block loaded yet
		m_docnoBlk = m_docnoStorage.load( docno_);
		if (!m_docnoBlk)
		{
			m_docnoItr = 0;
			return 0;
		}
		m_docnoItr = m_docnoBlk->upper_bound( docno_, m_docnoBlk->begin());
	}
	else
	{
		if (m_docnoBlk->isThisBlockAddress( docno_))
		{
			// [B] Answer in same block as for the last query
			if (!m_docnoItr || docno_ < m_docnoItr->docno())
			{
				m_docnoItr = m_docnoBlk->ptr();
			}
			m_docnoItr = m_docnoBlk->upper_bound( docno_, m_docnoItr);
		}
		else if (m_docnoBlk->isFollowBlockAddress( docno_))
		{
			// [C] Try to get answer from a follow block
			do
			{
				m_docnoBlk = m_docnoStorage.loadNext();
				if (m_docnoBlk)
				{
					Statistics::increment( Statistics::DocnoBlockReadBlockFollow);

					if (m_docnoBlk->id() < docno_ && !m_docnoBlk->isFollowBlockAddress( docno_))
					{	
						m_docnoBlk = m_docnoStorage.load( docno_);
						if (m_docnoBlk)
						{
							Statistics::increment( Statistics::DocnoBlockReadBlockRandom);
						}
						else
						{
							Statistics::increment( Statistics::DocnoBlockReadBlockRandomMiss);
							m_docnoItr = 0;
							return 0;
						}
					}
				}
				else
				{
					Statistics::increment( Statistics::DocnoBlockReadBlockFollowMiss);
					m_docnoItr = 0;
					return 0;
				}
			}
			while (m_docnoBlk->id() < docno_);

			m_docnoItr = m_docnoBlk->upper_bound( docno_, m_docnoBlk->begin());
		}
		else
		{
			// [D] Answer is in a 'far away block'
			m_docnoBlk = m_docnoStorage.load( docno_);
			if (!m_docnoBlk)
			{
				Statistics::increment( Statistics::DocnoBlockReadBlockRandomMiss);
				m_docnoItr = 0;
				return 0;
			}
			else
			{
				Statistics::increment( Statistics::DocnoBlockReadBlockRandom);
				m_docnoItr = m_docnoBlk->upper_bound( docno_, m_docnoBlk->begin());
			}
		}
	}
	if (!m_docnoItr) return 0;
	return m_docnoItr->docno();
}

Index PostingIterator::skipDocPosinfoBlock( const Index& docno_)
{
	m_positionScanner.clear();
	if (!m_posinfoBlk)
	{
		// [A] No block loaded yet
		m_posinfoBlk = m_posinfoStorage.load( docno_);
		if (!m_posinfoBlk)
		{
			m_posinfoItr = 0;
			return 0;
		}
		else
		{
			m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
		}
	}
	else
	{
		if (m_posinfoBlk->isThisBlockAddress( docno_))
		{
			// [B] Answer in same block as for the last query
			if (docno_ < m_posinfoBlk->docno_at( m_posinfoItr))
			{
				m_posinfoItr = m_posinfoBlk->begin();
			}
			else
			{
				m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoItr);
			}
		}
		else if (m_posinfoBlk->isFollowBlockAddress( docno_))
		{
			// [C] Try to get answer from a follow block
			do
			{
				m_posinfoBlk = m_posinfoStorage.loadNext();
				if (m_posinfoBlk)
				{
					Statistics::increment( Statistics::PosinfoBlockReadBlockFollow);

					if (m_posinfoBlk->id() < docno_ && !m_posinfoBlk->isFollowBlockAddress( docno_))
					{
						m_posinfoBlk = m_posinfoStorage.load( docno_);
						if (m_posinfoBlk)
						{
							Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
						}
						else
						{
							Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
							m_posinfoItr = 0;
							return 0;
						}
					}
				}
				else
				{
					Statistics::increment( Statistics::PosinfoBlockReadBlockFollowMiss);
					m_posinfoItr = 0;
					return 0;
				}
			}
			while (m_posinfoBlk->id() < docno_);

			m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
		}
		else
		{
			// [D] Answer is in a 'far away' block
			m_posinfoBlk = m_posinfoStorage.load( docno_);
			if (!m_posinfoBlk)
			{
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandomMiss);
				m_posinfoItr = 0;
				return 0;
			}
			else
			{
				Statistics::increment( Statistics::PosinfoBlockReadBlockRandom);
				m_posinfoItr = m_posinfoBlk->upper_bound( docno_, m_posinfoBlk->begin());
			}
		}
	}
	if (!m_posinfoItr || m_posinfoItr == m_posinfoBlk->end()) return 0;
	return m_posinfoBlk->docno_at( m_posinfoItr);
}

Index PostingIterator::skipDoc( const Index& docno_)
{
	if (m_docno)
	{
		if (docno_ == m_docno)
		{
			// [A] same as the current position
			return m_docno;
		}
		if (m_last_docno <= docno_ && m_docno > docno_)
		{
			// [B] same response as for the last query
			return m_docno;
		}
	}
	if (m_posinfoBlk)
	{
		m_docno = skipDocPosinfoBlock( docno_);
		m_last_docno = docno_;
		return m_docno;
	}
	else
	{
		m_positionScanner.clear();
		m_docno = skipDocDocnoBlock( docno_);
		m_last_docno = docno_;
		return m_docno;
	}
}

Index PostingIterator::skipPos( const Index& firstpos_)
{
	if (!m_posinfoItr)
	{
		if (!m_docno) return 0;
		if (m_docno != skipDocPosinfoBlock( m_docno))
		{
			throw std::runtime_error( "position information not available (do insert with position information for this type of query)");
		}
	}
	if (!m_positionScanner.initialized())
	{
		m_positionScanner = m_posinfoBlk->positionScanner_at( m_posinfoItr);
		if (!m_positionScanner.initialized()) return 0;
	}
	Index rt = m_positionScanner.skip( firstpos_);
	if (rt < firstpos_ && rt != 0)
	{
		throw std::logic_error("CRAZY posinfo iterator");
	}
	return rt;
}

unsigned int PostingIterator::frequency()
{
	if (m_posinfoItr)
	{
		return m_posinfoBlk->frequency_at( m_posinfoItr);
	}
	else
	{
		return m_docnoItr?m_docnoItr->ff():0;
	}
}

Index PostingIterator::documentFrequency()
{
	if (m_documentFrequency < 0)
	{
		KeyValueStorage dfstorage(
			m_db, DatabaseKey::DocFrequencyPrefix, false);
		const KeyValueStorage::Value* dfpacked
			= dfstorage.load( BlockKey( m_termtypeno, m_termvalueno));

		if (!dfpacked) return 0;
		char const* cc = dfpacked->ptr();
		m_documentFrequency = unpackIndex( cc, cc + dfpacked->size());
	}
	return m_documentFrequency;
}

