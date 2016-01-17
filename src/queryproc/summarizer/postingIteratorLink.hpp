/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_POSTING_ITERATOR_LINK_HPP_INCLUDED
#define _STRUS_POSTING_ITERATOR_LINK_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"

namespace strus
{

class PostingIteratorLink
	:public PostingIteratorInterface
{
public:
	PostingIteratorLink( PostingIteratorInterface* ref_)
		:m_ref(ref_){}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_ref->skipDoc( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return m_ref->skipDocCandidate( docno_);
	}

	virtual Index skipPos( const Index& firstpos)
	{
		return m_ref->skipPos( firstpos);
	}

	virtual const char* featureid() const
	{
		return m_ref->featureid();
	}

	virtual Index documentFrequency() const
	{
		return m_ref->documentFrequency();
	}

	virtual unsigned int frequency()
	{
		return m_ref->frequency();
	}

	virtual Index docno() const
	{
		return m_ref->docno();
	}

	virtual Index posno() const
	{
		return m_ref->posno();
	}

private:
	PostingIteratorInterface* m_ref;
};

}//namespace
#endif

