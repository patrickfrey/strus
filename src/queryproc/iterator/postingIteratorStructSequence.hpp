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
#ifndef _STRUS_ITERATOR_STRUCT_SEQUENCE_HPP_INCLUDED
#define _STRUS_ITERATOR_STRUCT_SEQUENCE_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class IteratorStructSequence
/// \brief Selects all elements that are appearing in a strict ascending position order inside a defined range without overlapping with a delimiter element.
class IteratorStructSequence
	:public IteratorJoin
{
public:
	/// \param[in] range_ the maximum position difference between the start element and the end element of the group
	/// \param[in] argitr the elements of this join 
	/// \param[in] with_cut_ true, if the first element of args is the cut element
	IteratorStructSequence(
			int range_, 
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			bool with_cut_,
			ErrorBufferInterface* errorhnd_);

	virtual ~IteratorStructSequence();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipPos( const Index& pos);

	virtual std::vector<const PostingIteratorInterface*>
			subExpressions( bool positive) const;

	virtual GlobalCounter documentFrequency() const;

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

private:
	Index m_docno;							///< current document number
	Index m_docno_cut;						///< next document number after m_docno that contains a cut element
	Index m_posno;							///< current position
	std::vector<Reference< PostingIteratorInterface> > m_argar;	///< arguments
	Reference<PostingIteratorInterface> m_cut;			///< the set of elements then must not appear inside the group
	bool m_with_cut;						///< true, if a cut variable is used
	int m_range;							///< the maximum position difference between the start element and the end element of the sequence
	std::string m_featureid;					///< unique id of the feature expression
	mutable GlobalCounter m_documentFrequency;			///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinStructSequence
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinStructSequence( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinStructSequence(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* getDescription() const
	{
		return _TXT("Get the set of postings (d,p) that exist in the second argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| < |rj| for i<j and i>2. Additionally there must not exist a posting in the first argument set that is overlapped by the interval formed by the other argument postings");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

class PostingJoinSequence
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinSequence( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinSequence(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* getDescription() const
	{
		return _TXT("Get the set of postings (d,p) that exist in the first argument set and (d,p+ri) exist in the argument set i with |ri| <= |range| and |ri| < |rj| for i<j");
	}

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


