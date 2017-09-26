/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "structureIterator.hpp"
#include "postingIteratorHelpers.hpp"

using namespace strus;

void StructureIterator::init( Index windowsize_, PostingIteratorInterface** valid_structar_, std::size_t structarSize_)
{
	m_valid_structar = valid_structar_;
	m_structarSize = structarSize_;
	m_windowsize = windowsize_;
	m_cur.first = 0;
	m_cur.second = 0;
}

std::pair<Index,Index> StructureIterator::skipPos( const Index& posno)
{
	if (!posno) return std::pair<Index,Index>();
	if (posno < m_cur.second && posno >= m_cur.first) return m_cur;

	if (m_cur.second && m_cur.second <= posno && m_cur.second + m_windowsize >= posno)
	{
		// ... we are close enough to the last structure element and skip seeking for it:
		m_cur.first = m_cur.second;
		for (;;)
		{
			m_cur.second = callSkipPos( m_cur.first+1, m_valid_structar, m_structarSize);
			if (m_cur.second && m_cur.second <= posno)
			{
				m_cur.first = m_cur.second;
				continue;
			}
			break;
		}
	}
	else
	{
		Index windowsize = m_windowsize;
		Index startpos;
		do
		{
			// Search start of structure frame:
			startpos = posno > windowsize ? (posno - windowsize):0;
			Index pos = callSkipPos( startpos, m_valid_structar, m_structarSize);
			if (pos && pos <= posno)
			{
				m_cur.first = pos;
				for (;;)
				{
					// Search end of structure frame:
					m_cur.second = callSkipPos( m_cur.first+1, m_valid_structar, m_structarSize);
					if (m_cur.second && m_cur.second <= posno)
					{
						m_cur.first = m_cur.second;
						continue;
					}
					break;
				}
				break;
			}
			else
			{
				windowsize = windowsize * 2 + 1;
			}
		}
		while (startpos);
	}
	return m_cur;
}

