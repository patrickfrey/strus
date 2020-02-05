/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "sentenceIterator.hpp"
#include "postingIteratorHelpers.hpp"
#include "private/internationalization.hpp"
#include <limits>

using namespace strus;

SentenceIterator::SentenceIterator( PostingIteratorInterface* eos_iter_, strus::Index docno, const strus::IndexRange& field_, strus::Index maxSentenceSize_)
	:m_eos_iter(eos_iter_)
	,m_field(field_)
	,m_cur()
	,m_maxSentenceSize(maxSentenceSize_)
	,m_eof(false)
{
	if (!m_eos_iter) throw std::runtime_error(_TXT("undefined end of sentence posting iterator"));
	if (!m_eos_iter->skipDoc( docno)) m_eof = true;
	m_cur.init( m_field.start(), m_field.start());
}

strus::IndexRange SentenceIterator::next()
{
	while (!m_eof)
	{
		strus::Index nextpos = m_eos_iter->skipPos( m_cur.end()+1);
		if (!nextpos || (m_field.defined() && nextpos > m_field.end()))
		{
			m_cur.init( m_cur.end()+1, m_field.defined() ? m_field.end() :  m_cur.end()+1+m_maxSentenceSize);
			m_eof = true;
			break;
		}
		else
		{
			strus::Index len = nextpos - (m_cur.end()+1);
			if (len <= 0 || len > m_maxSentenceSize)
			{
				m_cur.setEnd( nextpos);
				continue;
			}
			else
			{
				m_cur.init( m_cur.end()+1, nextpos);
				return m_cur;
			}
		}
	}
	m_cur.clear();
	return m_cur;
}



