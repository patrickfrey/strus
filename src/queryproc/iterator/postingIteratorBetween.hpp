/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_ITERATOR_BETWEEN_HPP_INCLUDED
#define _STRUS_ITERATOR_BETWEEN_HPP_INCLUDED
#include "strus/postingJoinOperatorInterface.hpp"
#include "postingIteratorJoin.hpp"
#include "docnoAllMatchItr.hpp"

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Iterator that selects the position from the first argument iterator that are between the second argument iterator and the third argument iterator
class IteratorBetween
	:public IteratorJoin
{
public:
	IteratorBetween( 
		const std::vector<Reference< PostingIteratorInterface> >& args,
		ErrorBufferInterface* errorhnd_);
	virtual ~IteratorBetween();

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

	virtual Index length() const
	{
		return m_elemitr->length();
	}

protected:
	Index m_docno;					///< current document number
	Index m_posno;					///< current position
	IndexRange m_field;				///< current field
	Reference< PostingIteratorInterface> m_elemitr;	///< select iterator
	Reference< PostingIteratorInterface> m_startitr;///< start iterator
	Reference< PostingIteratorInterface> m_enditr;	///< end iterator
	DocnoAllMatchItr m_docnoAllMatchItr;		///< document all match joiner
	std::string m_featureid;			///< unique id of the feature expression
	mutable Index m_documentFrequency;		///< document frequency (of the rarest subexpression)
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

} //namespace
#endif


