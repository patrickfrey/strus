/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#define _STRUS_ITERATOR_INTERSECT_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"
#include "positionWindow.hpp"
#include "docnoMatchPrioQueue.hpp"
#include "docnoAllMatchItr.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorIntersect
	:public IteratorJoin
{
public:
	IteratorIntersect(
			const std::vector<Reference< PostingIteratorInterface> >& args,
			ErrorBufferInterface* errorhnd_);
	virtual ~IteratorIntersect();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos);

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual Index length() const;

private:
	Index m_docno;							///< current document number
	Index m_posno;							///< current position
	std::vector<Reference< PostingIteratorInterface> > m_argar;	///< arguments
	DocnoAllMatchItr m_docnoAllMatchItr;				///< document all match joiner
	std::string m_featureid;					///< unique id of the feature expression
	mutable Index m_documentFrequency;				///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class IteratorIntersectWithCardinality
	:public IteratorJoin
{
public:
	IteratorIntersectWithCardinality(
			const std::vector<Reference< PostingIteratorInterface> >& args,
			unsigned int cardinality_,
			ErrorBufferInterface* errorhnd_);
	virtual ~IteratorIntersectWithCardinality();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos);

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual Index length() const;

private:
	/// \brief We use fixed size arrays and restrict the maximum number of features to a reasonable amount.
	Index m_docno;							///< current document
	Index m_posno;							///< current position
	Index m_call_posno;						///< previous position
	unsigned int m_cardinality;					///< cardinality
	std::vector<Reference< PostingIteratorInterface> > m_argar;	///< arguments
	DocnoMatchPrioQueue m_docnoMatchPrioQueue;
	PositionWindow m_positionWindow;
	bool m_windowIsInitialized;					///< true, iff the window is initialized for the current document number
	std::string m_featureid;					///< unique id of the feature expression
	mutable Index m_documentFrequency;				///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinIntersect
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinIntersect( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinIntersect(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference<PostingIteratorInterface> >& itrs,
			int range,
			unsigned int cardinality) const;

	virtual const char* name() const {return "intersect";}

	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


