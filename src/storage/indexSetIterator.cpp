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

IndexSetIterator::IndexSetIterator( DatabaseClientInterface* database_, DatabaseKey::KeyPrefix dbprefix_, const BlockKey& key_)
	:m_dbadapter( database_, (char)dbprefix_, key_)
	,m_elemBlk()
	,m_elemno(0)
{}

bool IndexSetIterator::loadBlock( const Index& elemno_)
{
	bool rt = true;
	if (!m_elemBlk.empty())
	{
		if (m_elemBlk.isFollowBlockAddress( elemno_))
		{
			m_elemCursor.reset();
			while ((rt=m_dbadapter.loadNext( m_elemBlk)) && elemno_ > m_elemBlk.id())
			{
				if (!m_elemBlk.isFollowBlockAddress( elemno_))
				{
					rt = m_dbadapter.loadUpperBound( elemno_, m_elemBlk);
					break;
				}
			}
		}
		else
		{
			m_elemCursor.reset();
			rt = m_dbadapter.loadUpperBound( elemno_, m_elemBlk);
		}
	}
	else
	{
		m_elemCursor.reset();
		rt = m_dbadapter.loadUpperBound( elemno_, m_elemBlk);
	}
	return rt;
}

Index IndexSetIterator::skip( const Index& elemno_)
{
	if (!m_elemno || !m_elemBlk.isThisBlockAddress( elemno_))
	{
		if (!loadBlock( elemno_)) return 0;
	}
	return m_elemno = m_elemBlk.skip( elemno_, m_elemCursor);
}

