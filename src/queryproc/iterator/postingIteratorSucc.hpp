/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_SUCCESSOR_HPP_INCLUDED
#define _STRUS_ITERATOR_SUCCESSOR_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorSucc
	:public IteratorJoin
{
public:
	IteratorSucc( const Reference< PostingIteratorInterface>& origin_, ErrorBufferInterface* errorhnd_)
		:m_origin( origin_)
		,m_featureid( origin_->featureid())
		,m_errorhnd(errorhnd_)
	{
		m_featureid.push_back('>');
	}

	virtual ~IteratorSucc()
	{}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_)
	{
		return m_origin->skipDoc( docno_);
	}

	virtual Index skipDocCandidate( const Index& docno_)
	{
		return m_origin->skipDocCandidate( docno_);
	}

	virtual Index skipPos( const Index& pos_)
	{
		Index rt = m_origin->skipPos( pos_);
		return rt?(rt + 1):0;
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

	virtual Index length() const
	{
		return m_origin->length();
	}

private:
	Reference<PostingIteratorInterface> m_origin;			///< base feature expression this is the successor of
	std::string m_featureid;					///< unique id of the feature expression
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};



class PostingJoinSucc
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinSucc( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinSucc(){}

	virtual const char* name() const {return "succ";}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range,
			unsigned int cardinality) const;

	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


