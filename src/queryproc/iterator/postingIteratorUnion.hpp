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
#include "private/bitOperations.hpp"
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
	class selected_iterator
	{
	public:
		selected_iterator( uint64_t set_, std::vector<Reference<PostingIteratorInterface> >::iterator aitr_)
			:m_aitr(aitr_),m_set(set_),m_idx(0)
		{
			skip();
		}
		selected_iterator( const selected_iterator& o)
			:m_aitr(o.m_aitr),m_set(o.m_set),m_idx(o.m_idx){}

		selected_iterator& operator++()				{skip(); return *this;}
		selected_iterator operator++(int)			{selected_iterator rt(*this); skip(); return rt;}

		bool operator==( const selected_iterator& o) const	{return m_set==o.m_set && m_idx==o.m_idx;}
		bool operator!=( const selected_iterator& o) const	{return m_set!=o.m_set || m_idx!=o.m_idx;}

		PostingIteratorInterface& operator*() const		{return *(m_aitr+m_idx-1)->get();}
		PostingIteratorInterface* operator->() const		{return (m_aitr+m_idx-1)->get();}

	private:
		void skip()
		{
			m_idx = BitOperations::bitScanForward( m_set);
			if (m_idx)
			{
				m_set -= (uint64_t)1<<(m_idx-1);
			}
		}

	private:
		std::vector<Reference<PostingIteratorInterface> >::iterator m_aitr;
		uint64_t m_set;
		std::size_t m_idx;
	};

	selected_iterator selected_begin()
	{
		return selected_iterator( m_selected, m_argar.begin());
	}

	selected_iterator selected_end()
	{
		return selected_iterator( 0, m_argar.begin());
	}

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
		m_selected |= ((uint64_t)1 << idx);
	}
	void unsetSelected( unsigned int idx)
	{
		m_selected ^= ((uint64_t)1 << idx);
	}
	void clearSelected()
	{
		m_selected = 0;
	}

private:
	Index m_docno;
	Index m_posno;							///< current position
	std::vector<Reference<PostingIteratorInterface> > m_argar;	///< arguments
	uint64_t m_selected;						///< set pf bits parallel to arguments that specifies the current document matches of the arguments
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


