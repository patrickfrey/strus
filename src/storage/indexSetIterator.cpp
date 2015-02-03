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
#include "indexSetIterator.hpp"

using namespace strus;

IndexSetIterator::IndexSetIterator( DatabaseInterface* database_, DatabaseKey::KeyPrefix dbprefix_, const BlockKey& key_)
	:m_dbadapter( database_, (char)dbprefix_, key_)
	,m_elemBlk()
	,m_elemItr(0)
	,m_elemno(0)
	,m_range_from(0)
	,m_range_to(0)
{}

bool IndexSetIterator::loadNext()
{
	if (!m_dbadapter.loadNext( m_elemBlk))
	{
		m_elemItr = 0;
		return false;
	}
	else
	{
		return true;
	}
}

bool IndexSetIterator::loadBlock( const Index& elemno_)
{
	if (!m_elemBlk.empty())
	{
		if (m_elemBlk.isThisBlockAddress( elemno_))
		{
			if (m_elemItr)
			{
				Index from_,to_;
				char const* eitr = m_elemItr;
				if (!m_elemBlk.getNextRange( eitr, from_, to_))
				{
					m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
				}
				else if (elemno_ < from_)
				{
					m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
				}
				else if (elemno_ > to_)
				{
					m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemItr);
				}
				else
				{
					m_range_from = from_;
					m_range_to = to_;
					m_elemItr = eitr;
					return true;
				}
			}
			else
			{
				m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
			}
		}
		else if (m_elemBlk.isFollowBlockAddress( elemno_))
		{
			while (loadNext())
			{
				if (elemno_ > m_elemBlk.id())
				{	
					if (!m_elemBlk.isFollowBlockAddress( elemno_))
					{
						if (m_dbadapter.loadUpperBound( elemno_, m_elemBlk))
						{
							m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
						}
						else
						{
							m_elemItr = 0;
						}
						break;
					}
				}
				else
				{
					m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
					break;
				}
			}
		}
		else
		{
			if (m_dbadapter.loadUpperBound( elemno_, m_elemBlk))
			{
				m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
			}
			else
			{
				m_elemItr = 0;
			}
		}
	}
	else
	{
		if (m_dbadapter.loadUpperBound( elemno_, m_elemBlk))
		{
			m_elemItr = m_elemBlk.upper_bound( elemno_, m_elemBlk.charptr());
		}
		else
		{
			m_elemItr = 0;
		}
	}
	if (!m_elemItr || !m_elemBlk.getNextRange( m_elemItr, m_range_from, m_range_to))
	{
		return false;
	}
	return true;
}

Index IndexSetIterator::skip( const Index& elemno_)
{
	if (m_elemBlk.empty() || m_range_from > elemno_ || m_range_to < elemno_)
	{
		if (!loadBlock( elemno_)) return 0;
	}
	if (m_range_from >= elemno_)
	{
		return m_elemno = m_range_from;
	}
	if (m_range_from <= elemno_ && m_range_to >= elemno_)
	{
		return m_elemno = elemno_;
	}
	else
	{
		return 0;
	}
}

