/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_SEQUENCE_IMM_HPP_INCLUDED
#define _STRUS_ITERATOR_SEQUENCE_IMM_HPP_INCLUDED
#include "postingIteratorJoin.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "private/internationalization.hpp"
#include "docnoAllMatchItr.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class IteratorStructSequence
/// \brief Selects all elements that are appearing in a strict ascending position order immediately following each other (next.pos = prev.pos + prev.len).
class IteratorSequenceImm
	:public IteratorJoin
{
public:
	/// \param[in] range_ the maximum position difference between the start element and the end element of the group
	/// \param[in] argitr the elements of this join 
	/// \param[in] with_cut_ true, if the first element of args is the cut element
	IteratorSequenceImm(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			ErrorBufferInterface* errorhnd_);

	virtual ~IteratorSequenceImm();

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}
	virtual Index skipDoc( const Index& docno);
	virtual Index skipDocCandidate( const Index& docno_);
	virtual Index skipPos( const Index& pos);

	virtual GlobalCounter documentFrequency() const;

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
		return (m_posno && m_argar.size())?(m_argar.back()->posno() - m_posno + m_argar.back()->length()):0;
	}


private:
	Index m_docno;							///< current document number
	Index m_posno;							///< current position
	std::vector<Reference< PostingIteratorInterface> > m_argar;	///< arguments
	DocnoAllMatchItr m_docnoAllMatchItr;				///< document all match joiner
	std::string m_featureid;					///< unique id of the feature expression
	mutable GlobalCounter m_documentFrequency;			///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

class PostingJoinSequenceImm
	:public PostingJoinOperatorInterface
{
public:
	explicit PostingJoinSequenceImm( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}
	virtual ~PostingJoinSequenceImm(){}

	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference< PostingIteratorInterface> >& argitr,
			int range_,
			unsigned int cardinality_) const;

	virtual const char* name() const {return "sequence_imm";}

	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif


