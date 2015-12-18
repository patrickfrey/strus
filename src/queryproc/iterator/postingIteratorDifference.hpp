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
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorDifference
	:public IteratorJoin
{
public:
	IteratorDifference(
			const Reference<PostingIteratorInterface>& positive_,
			const Reference<PostingIteratorInterface>& negative_,
			ErrorBufferInterface* errorhnd_);
	virtual ~IteratorDifference();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const;

	virtual GlobalCounter documentFrequency() const
	{
		return m_positive->documentFrequency();
	}

	virtual Index docno() const
	{
		return m_positive->docno();
	}

	virtual Index posno() const
	{
		return m_positive->posno();
	}

private:
	Index m_docno;
	Index m_docno_neg;
	Reference<PostingIteratorInterface> m_positive;
	Reference<PostingIteratorInterface> m_negative;
	std::string m_featureid;				///< unique id of the feature expression
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


class PostingJoinDifference
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinDifference( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinDifference(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference<PostingIteratorInterface> >& argitr,
			int range,
			unsigned int cardinality) const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};

}//namespace
#endif


