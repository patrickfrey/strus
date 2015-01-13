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
#ifndef _STRUS_ITERATOR_DIFFERENCE_HPP_INCLUDED
#define _STRUS_ITERATOR_DIFFERENCE_HPP_INCLUDED
#include "iterator/postingIteratorJoin.hpp"
#include "postingIteratorReference.hpp"

namespace strus
{

class IteratorDifference
	:public IteratorJoin
{
public:
	IteratorDifference( const IteratorDifference& o);
	IteratorDifference( const PostingIteratorInterface* positive_, const PostingIteratorInterface* negative_);
	virtual ~IteratorDifference(){}

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const;

	virtual Index documentFrequency() const
	{
		return m_positive.get()?m_positive->documentFrequency():0;
	}

	virtual Index docno() const
	{
		return m_positive.get()?m_positive->docno():0;
	}

	virtual Index posno() const
	{
		return m_positive.get()?m_positive->posno():0;
	}

	virtual PostingIteratorInterface* copy() const
	{
		return new IteratorDifference( *this);
	}

private:
	Index m_docno;
	Index m_docno_neg;
	PostingIteratorReference m_positive;
	PostingIteratorReference m_negative;
	std::string m_featureid;		///< unique id of the feature expression
};

}//namespace
#endif


