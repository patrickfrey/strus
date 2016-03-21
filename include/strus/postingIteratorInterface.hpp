/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for an iterator on postings (pairs of (d,p) where d is the document number and p the position of a feature)
/// \file "postingIteratorInterface.hpp"
#ifndef _STRUS_POSTING_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_POSTING_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>

namespace strus
{

/// \class PostingIteratorInterface
/// \brief Structure that represents a set of feature occurrencies (postings) as iterator.
class PostingIteratorInterface
{
public:
	virtual ~PostingIteratorInterface(){}

	/// \brief Return the next document match with a document number higher than or equal to a given document number
	/// \param[in] docno the minimum document number to fetch
	virtual Index skipDoc( const Index& docno)=0;

	/// \brief Return a candidate with a document number higher than or equal to a given document number without guarantee, that the document has matching positions
	/// \note Used for optimizing complex join operations that would first like to make a join on the document sets, before looking at the positions
	/// \param[in] docno the minimum document number to fetch
	virtual Index skipDocCandidate( const Index& docno)=0;

	/// \brief Return the next matching position higher than or equal to firstpos in the current document. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	/// \param[in] firstpos the minimum position to fetch
	virtual Index skipPos( const Index& firstpos)=0;

	/// \brief Unique id in the system for a feature expression used for debugging and tracing
	/// \return the id string
	virtual const char* featureid() const=0;

	/// \brief Get the number of documents where the feature occurrs
	/// \remark May not be defined exactly for composed features. In this case a substitute value should be returned, estimated from the df's of the sub expressions
	/// \return the document frequency (aka 'df')
	virtual Index documentFrequency() const=0;

	/// \brief Get the frequency of the feature in the current document
	/// \return the feature frequency (aka 'ff' of 'tf')
	virtual unsigned int frequency()=0;

	/// \brief Get the current document number
	/// \return the document number
	virtual Index docno() const=0;

	/// \brief Get the current position number
	/// \return the position number
	virtual Index posno() const=0;
};

}//namespace
#endif


