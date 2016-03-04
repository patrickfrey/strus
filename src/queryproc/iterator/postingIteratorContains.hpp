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
#ifndef _STRUS_ITERATOR_CONTAINS_HPP_INCLUDED
#define _STRUS_ITERATOR_CONTAINS_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "docnoMatchPrioQueue.hpp"
#include "docnoAllMatchItr.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorContains
	:public IteratorJoin
{
public:
	IteratorContains( 
		const std::vector<Reference< PostingIteratorInterface> >& args,
		ErrorBufferInterface* errorhnd_);
	virtual ~IteratorContains();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos)
	{
		if (pos > 1) return m_posno=0;
		return m_posno=1;
	}

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

protected:
	Index m_docno;
	Index m_posno;					///< current position
	DocnoAllMatchItr m_docnoAllMatchItr;		///< document all match joiner
	std::string m_featureid;			///< unique id of the feature expression
	mutable Index m_documentFrequency;		///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

class IteratorContainsWithCardinality
	:public IteratorJoin
{
public:
	IteratorContainsWithCardinality(
		const std::vector<Reference< PostingIteratorInterface> >& args,
		unsigned int cardinality_,
		ErrorBufferInterface* errorhnd_);
	virtual ~IteratorContainsWithCardinality(){}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos)
	{
		if (pos > 1) return m_posno=0;
		return m_posno=1;
	}

	virtual Index documentFrequency() const;

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
	Index m_posno;					///< current position
	DocnoMatchPrioQueue m_prioqueue;		///< priority queue for iterating on matches
	std::string m_featureid;			///< unique id of the feature expression
	mutable Index m_documentFrequency;		///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


class PostingJoinContains
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinContains( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinContains(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference<PostingIteratorInterface> >& itrs,
			int range,
			unsigned int cardinality) const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


