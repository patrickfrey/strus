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

	virtual Index skipDoc( const Index& docno)
	{
		if (m_itr < m_end)
		{
			if (*m_itr > docno)
			{
				if (m_itr > m_start)
				{
					--m_itr;
					if (*m_itr < docno)
					{
						++m_itr;
						return *m_itr;
					}
				}
				m_itr = m_start;
			}
		}
		while (m_itr < m_end && *m_itr < docno)
		{
			++m_itr;
		}
		return (m_itr == m_end)?0:*m_itr;
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
	std::size_t m_size;
	std::vector<Index>::const_iterator m_start;
	std::vector<Index>::const_iterator m_itr;
	std::vector<Index>::const_iterator m_end;
};

}//namespace

#endif



