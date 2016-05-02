/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_UNION_HPP_INCLUDED
#define _STRUS_ITERATOR_UNION_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include <vector>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

class IteratorUnion
	:public IteratorJoin
{
public:
	IteratorUnion( const std::vector<Reference< PostingIteratorInterface> >& args_, ErrorBufferInterface* errorhnd_);
	virtual ~IteratorUnion();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos_);

	virtual Index documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

public:
	/// \brief The maximum number of arguments is fixed, because of the 64bit integer used for the current selection set
	enum {MaxNofElements=64};

protected:
	const PostingIteratorInterface* arg( unsigned int idx) const
	{
		return m_argar[ idx].get();
	}

	PostingIteratorInterface* arg( unsigned int idx)
	{
		return m_argar[ idx].get();
	}

	unsigned int nofargs() const
	{
		return m_argar.size();
	}

private:
	void setSelected( unsigned int idx)
	{
		m_selected.set( idx);
	}
	void unsetSelected( unsigned int idx)
	{
		m_selected.unset( idx);
	}
	void clearSelected()
	{
		m_selected.reset();
	}

private:
	Index m_docno;
	Index m_posno;							///< current position
	std::vector<Reference<PostingIteratorInterface> > m_argar;	///< arguments
	strus::utils::BitSet m_selected;
	std::string m_featureid;					///< unique id of the feature expression
	mutable Index m_documentFrequency;				///< document frequency (of the most frequent subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinUnion
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinUnion( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinUnion(){}

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


