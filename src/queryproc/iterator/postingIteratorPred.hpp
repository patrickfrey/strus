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
#ifndef _STRUS_ITERATOR_PREDECCESSOR_HPP_INCLUDED
#define _STRUS_ITERATOR_PREDECCESSOR_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorPred
	:public IteratorJoin
{
public:
	IteratorPred( const Reference< PostingIteratorInterface>& origin_, ErrorBufferInterface* errorhnd_)
		:m_origin( origin_)
		,m_featureid(origin_->featureid())
		,m_errorhnd(errorhnd_)
	{
		m_featureid.push_back('<');
	}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual ~IteratorPred()
	{}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_origin->skipDoc( docno_);
	}

	virtual Index skipPos( const Index& pos_)
	{
		Index rt = m_origin->skipPos( pos_+1);
		return rt?(rt - 1):0;
	}

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const
	{
		return m_origin->subExpressions( positive);
	}

	virtual GlobalCounter documentFrequency() const
	{
		return m_origin->documentFrequency();
	}

	virtual Index docno() const
	{
		return m_origin->docno();
	}

	virtual Index posno() const
	{
		return m_origin->posno();
	}

private:
	Reference<PostingIteratorInterface> m_origin;			///< base feature expression this is the predeccessor of
	std::string m_featureid;					///< unique id of the feature expression
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinPred
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinPred( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinPred(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range,
			unsigned int cardinality) const;

	virtual const char* getDescription() const
	{
		return _TXT("Get the set of postings (d,p-1) for all (d,p) with p>1 in the argument set");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

