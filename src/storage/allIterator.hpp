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
#ifndef _STRUS_LVDB_ALL_ITERATOR_HPP_INCLUDED
#define _STRUS_LVDB_ALL_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"

namespace strus {

/// \brief Iterator representing the complete set of document numbers
class AllIterator
	:public PostingIteratorInterface
{
public:
	explicit AllIterator( Index maxDocno_)
		:m_maxDocno(maxDocno_),m_docno(0){}

	virtual ~AllIterator(){}

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const
	{
		return std::vector<const PostingIteratorInterface*>();
	}

	virtual const char* featureid() const
	{
		return "*";
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return skipDocImpl( docno_);
	}

	virtual Index skipPos( const Index& posno_)
	{
		return 0;
	}

	virtual unsigned int frequency()
	{
		return 0;
	}

	virtual Index documentFrequency() const
	{
		return m_maxDocno;
	}
	
	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return 0;
	}

private:
	Index skipDocImpl( const Index& docno_)
	{
		if (docno_ <= m_maxDocno)
		{
			if (docno_)
			{
				return m_docno = docno_;
			}
			else
			{
				return m_docno = 1;
			}
		}
		return 0;
	}

private:
	Index m_maxDocno;
	Index m_docno;
};

}
#endif
