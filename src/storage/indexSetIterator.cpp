/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "indexSetIterator.hpp"

using namespace strus;

IndexSetIterator::IndexSetIterator( const DatabaseClientInterface* database_, DatabaseKey::KeyPrefix dbprefix_, const BlockKey& key_, bool useCache_)
	:m_dbadapter( (char)dbprefix_, database_, key_, useCache_)
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

