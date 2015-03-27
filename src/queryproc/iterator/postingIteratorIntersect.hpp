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
#ifndef _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#define _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{

class IteratorIntersect
	:public IteratorJoin
{
public:
	IteratorIntersect( const std::vector<Reference< PostingIteratorInterface> >& args);
	virtual ~IteratorIntersect();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const PostingIteratorInterface*> subExpressions( bool positive) const;

	virtual GlobalCounter documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

private:
	Index m_docno;
	Index m_posno;							///< current position
	std::vector<Reference< PostingIteratorInterface> > m_argar;
	std::string m_featureid;					///< unique id of the feature expression
	mutable GlobalCounter m_documentFrequency;			///< document frequency (of the rarest subexpression)
};


class PostingJoinIntersect
	:public PostingJoinOperatorInterface
{
public:
	virtual ~PostingJoinIntersect(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference<PostingIteratorInterface> >& itrs,
			int range) const
	{
		if (range != 0) throw strus::runtime_error( _TXT( "no range argument expected"));
		if (itrs.size() == 0) throw strus::runtime_error( _TXT( "too few arguments"));

		return new IteratorIntersect( itrs);
	}
};

}//namespace
#endif


