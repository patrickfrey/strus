/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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


