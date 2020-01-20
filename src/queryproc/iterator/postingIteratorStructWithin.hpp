/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_STRUCT_WITHIN_HPP_INCLUDED
#define _STRUS_ITERATOR_STRUCT_WITHIN_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"
#include "docnoAllMatchItr.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class IteratorStructWithin
/// \brief Selects all elements that are appearing inside a defined range without overlapping with a structure delimiter element.
class IteratorStructWithin
	:public IteratorJoin
{
public:
	/// \param[in] range_ the maximum position difference between the start element and the end element of the group
	/// \param[in] args the elements of this join 
	/// \param[in] with_cut true, if the first element of args is the cut element
	IteratorStructWithin( int range_, const std::vector<Reference< PostingIteratorInterface> >& args, bool with_cut_, bool strict_, ErrorBufferInterface* errorhnd_);

	virtual ~IteratorStructWithin();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index skipDoc( const Index& docno);
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

	virtual Index length() const
	{
		return m_argar.size()?(m_argar.back()->posno() - m_posno + m_argar.back()->length()):0;
	}

private:
	Index positionCut( strus::Index minpos, strus::Index maxpos);

public:
	enum {MaxNofArguments=256};
private:
	Index m_docno;							///< current document number
	Index m_docno_cut;						///< next document number after m_docno that contains a cut element
	Index m_posno;							///< current position
	std::vector<Reference< PostingIteratorInterface> > m_argar;	///< arguments
	DocnoAllMatchItr m_docnoAllMatchItr;				///< document all match joiner
	Reference<PostingIteratorInterface> m_cut;			///< the set of elements then must not appear inside the group
	bool m_with_cut;						///< true, if a cut variable is used
	bool m_strict;							///< true, if all elements must have different positions
	int m_range;							///< the maximum position difference between the start element and the end element of the group
	std::string m_featureid;					///< unique id of the feature expression
	mutable Index m_documentFrequency;				///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};




class PostingJoinStructWithin
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinStructWithin( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinStructWithin(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* name() const {return "within_struct";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinWithin
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinWithin( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinWithin(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* name() const {return "within";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinStructInRange
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinStructInRange( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinStructInRange(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* name() const {return "inrange_struct";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


class PostingJoinInRange
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinInRange( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinInRange(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* name() const {return "inrange";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


}//namespace
#endif


