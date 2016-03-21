/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Iterator on postings defined by a set of document numbers (used for restrictions on a document set)
/// \file "docsetPostingIterator.hpp"
#ifndef _STRUS_DOCSET_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_DOCSET_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"

namespace strus
{
class DocsetPostingIterator
	:public PostingIteratorInterface
{
public:
	DocsetPostingIterator( const std::vector<Index>& ar)
		:m_size(ar.size()),m_start(ar.begin()),m_itr(ar.begin()),m_end(ar.end()){}
	DocsetPostingIterator( const DocsetPostingIterator& o)
		:m_size(o.m_size),m_itr(o.m_itr),m_end(o.m_end){}
	DocsetPostingIterator()
		:m_size(0){}

	virtual ~DocsetPostingIterator(){}

	virtual Index skipDoc( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipPos( const Index& firstpos)
	{
		return (m_itr == m_end && firstpos <= 1)?0:1;
	}

	virtual const char* featureid() const
	{
		return "_";
	}

	virtual std::vector<const PostingIteratorInterface*> subExpressions( bool positive) const
	{
		return std::vector<const PostingIteratorInterface*>();
	}

	virtual Index documentFrequency() const
	{
		return m_size;
	}

	virtual unsigned int frequency()
	{
		return (m_itr == m_end)?0:1;
	}

	virtual Index docno() const
	{
		return (m_itr == m_end)?0:*m_itr;
	}

	virtual Index posno() const
	{
		return (m_itr == m_end)?0:1;
	}

private:
	Index skipDocImpl( const Index& docno_)
	{
		if (m_itr < m_end)
		{
			if (*m_itr > docno_)
			{
				if (m_itr > m_start)
				{
					--m_itr;
					if (*m_itr < docno_)
					{
						++m_itr;
						return *m_itr;
					}
				}
				m_itr = m_start;
			}
		}
		while (m_itr < m_end && *m_itr < docno_)
		{
			++m_itr;
		}
		return (m_itr == m_end)?0:*m_itr;
	}

private:
	std::size_t m_size;
	std::vector<Index>::const_iterator m_start;
	std::vector<Index>::const_iterator m_itr;
	std::vector<Index>::const_iterator m_end;
};

}//namespace

#endif



