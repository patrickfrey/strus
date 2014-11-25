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
#ifndef _STRUS_ITERATOR_UNION_HPP_INCLUDED
#define _STRUS_ITERATOR_UNION_HPP_INCLUDED
#include "iterator/postingIteratorJoin.hpp"
#include "postingIteratorReference.hpp"
#include "bitOperations.hpp"

namespace strus
{

class IteratorUnion
	:public IteratorJoin
{
public:
	IteratorUnion( const IteratorUnion& o);
	IteratorUnion( std::size_t nofargs_, const PostingIteratorInterface** args_);

	virtual const std::string& featureid() const
	{
		return m_featureid;
	}

	virtual ~IteratorUnion(){}

	virtual Index skipDoc( const Index& docno_);
	virtual Index skipPos( const Index& pos_);

	virtual std::vector<PostingIteratorInterface*>
			subExpressions( bool positive);

	virtual Index documentFrequency();

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual PostingIteratorInterface* copy() const
	{
		return new IteratorUnion( *this);
	}

protected:
	class selected_iterator
	{
	public:
		selected_iterator( uint64_t set_, PostingIteratorReferenceArray& argar_)
			:m_ar(&argar_),m_set(set_),m_idx(0)
		{
			skip();
		}
		selected_iterator( const selected_iterator& o)
			:m_ar(o.m_ar),m_set(o.m_set),m_idx(o.m_idx){}
		selected_iterator()
			:m_ar(0),m_set(0),m_idx(0){}

		selected_iterator& operator++()				{skip(); return *this;}
		selected_iterator operator++(int)			{selected_iterator rt(*this); skip(); return rt;}

		bool operator==( const selected_iterator& o) const	{return m_set==o.m_set && m_idx==o.m_idx;}
		bool operator!=( const selected_iterator& o) const	{return m_set!=o.m_set || m_idx!=o.m_idx;}

		PostingIteratorInterface& operator*() const		{return (*m_ar)[ m_idx-1];}
		PostingIteratorInterface* operator->() const		{return &(*m_ar)[ m_idx-1];}

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
		PostingIteratorReferenceArray* m_ar;
		uint64_t m_set;
		std::size_t m_idx;
	};

	selected_iterator selected_begin()
	{
		return selected_iterator( m_selected, m_argar);
	}

	selected_iterator selected_end()
	{
		return selected_iterator();
	}

	const PostingIteratorInterface* arg( unsigned int idx) const
	{
		return &m_argar[ idx];
	}
	PostingIteratorInterface* arg( unsigned int idx)
	{
		return &m_argar[ idx];
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
	void clearSelected()
	{
		m_selected = 0;
	}

private:
	Index m_docno;
	Index m_posno;				///< current position
	PostingIteratorReferenceArray m_argar;	///< union arguments
	uint64_t m_selected;			///< set pf bits parallel to arguments that specifies the current document matches of the arguments
	std::string m_featureid;		///< unique id of the feature expression
	Index m_documentFrequency;		///< document frequency (of the most frequent subexpression)
};

}//namespace
#endif


