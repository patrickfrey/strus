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
#ifndef _STRUS_ITERATOR_UNION_HPP_INCLUDED
#define _STRUS_ITERATOR_UNION_HPP_INCLUDED
#include "iterator/postingIteratorJoin.hpp"
#include "postingIteratorReference.hpp"

namespace strus
{

class IteratorUnion
	:public IteratorJoin
{
public:
	IteratorUnion( const IteratorUnion& o);
	IteratorUnion( std::size_t nofargs_, const PostingIteratorInterface** args_);

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}

	virtual ~IteratorUnion(){}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipPos( const Index& pos_);

	virtual std::vector<PostingIteratorInterface*>
			subExpressions( bool positive);

	virtual Index documentFrequency();

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual PostingIteratorInterface* copy() const
	{
		return new IteratorUnion( *this);
	}

protected:
	bool selected( unsigned int idx)
	{
		return m_selected[idx];
	}
	PostingIteratorInterface* arg( unsigned int idx) const
	{
		return m_argar[ idx].get();
	}
	unsigned int nofargs() const
	{
		return m_argar.size();
	}

private:
	Index m_docno;
	Index m_posno;					///< current position
	std::vector<PostingIteratorReference> m_argar;	///< union arguments
	std::vector<bool> m_selected;			///< parallel array to arguments that specifies if the current document matches the argument
	std::string m_featureid;			///< unique id of the feature expression
	Index m_documentFrequency;			///< document frequency (of the most frequent subexpression)
};

}//namespace
#endif


