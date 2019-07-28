/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos);

	virtual Index documentFrequency() const
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

	virtual Index length() const
	{
		return m_positive->length();
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

	virtual const char* name() const {return "diff";}

	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};

}//namespace
#endif


