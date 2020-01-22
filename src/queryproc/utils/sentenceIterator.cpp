/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "sentenceIterator.hpp"
#include "postingIteratorHelpers.hpp"

using namespace strus;

void SentenceIterator::init( Index windowsize_, PostingIteratorInterface** delimar_, std::size_t structarSize_)
{
	m_delimar = delimar_;
	m_structarSize = structarSize_;
	m_windowsize = windowsize_;
	m_cur.init( 0, 0);
}

strus::IndexRange SentenceIterator::skipPos( strus::Index posno)
{
	if (!posno) return strus::IndexRange();
	if (posno < m_cur.end() && posno >= m_cur.start()) return m_cur;

	if (m_cur.end() && m_cur.end() <= posno && m_cur.end() + m_windowsize >= posno)
	{
		// ... we are close enough to the last structure element and skip seeking for it:
		strus::Index start = m_cur.end();
		for (;;)
		{
			strus::Index end = callSkipPos( start+1, m_delimar, m_structarSize);
			if (end && end <= posno)
			{
				start = end;
				continue;
			}
			m_cur.init( start, end);
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
			Index pos = callSkipPos( startpos, m_delimar, m_structarSize);
			if (pos && pos <= posno)
			{
				strus::Index start = pos;
				for (;;)
				{
					// Search end of structure frame:
					strus::Index end = callSkipPos( start+1, m_delimar, m_structarSize);
					if (end && end <= posno)
					{
						start = end;
						continue;
					}
					m_cur.init( start, end);
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

