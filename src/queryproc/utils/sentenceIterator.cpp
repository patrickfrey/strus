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
	,m_maxSentenceSize(maxSentenceSize_)
{
	if (!m_eos_iter) throw std::runtime_error(_TXT("undefined end of sentence posting iterator"));
	if (m_eos_iter->skipDoc( docno) != docno) m_eos_iter = NULL;
}

strus::IndexRange SentenceIterator::skipPos( strus::Index pos)
{
	strus::IndexRange rt;
	if (!m_eos_iter) return rt;
	if (pos < m_field.start()) pos = m_field.start();
	for (;;)
	{
		strus::Index startpos = pos <= (m_field.start() + m_maxSentenceSize) ? m_field.start() : (pos - m_maxSentenceSize);
		strus::Index si = m_eos_iter->skipPos( startpos);
		for (; si && si < pos; startpos = si+1,si = m_eos_iter->skipPos( startpos)){}
		strus::Index endpos = m_eos_iter->skipPos( startpos+1);
		if (!endpos)
		{
			if (m_field.defined())
			{
				endpos = m_field.end();
				if (startpos < endpos) rt.init( startpos ? startpos : 1, endpos);
			}
			else
			{
				rt.init( startpos ? startpos : 1, startpos + m_maxSentenceSize);
			}
		}
		else if (m_field.defined() && !m_field.contain( endpos))
		{
			endpos = m_field.end();
			if (startpos < endpos) rt.init( startpos ? startpos : 1, endpos);
		}
		else if (endpos - startpos <= m_maxSentenceSize)
		{
			rt.init( startpos ? startpos : 1, endpos);
		}
		else
		{
			pos = endpos;
			continue;
		}
		break;
	}
	return rt;
}



