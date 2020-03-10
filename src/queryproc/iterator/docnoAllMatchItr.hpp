/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Iterator on the all document matches

#ifndef _STRUS_DOCNO_ALL_MATCH_ITR_HPP_INCLUDED
#define _STRUS_DOCNO_ALL_MATCH_ITR_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>

namespace strus
{

/// \brief Iterator on the all document matches
class DocnoAllMatchItr
{
public:
	typedef Reference< PostingIteratorInterface> PostingIteratorReference;

	/// \brief Constructor
	DocnoAllMatchItr( const std::vector<PostingIteratorReference>& args_);
	DocnoAllMatchItr( std::vector<PostingIteratorReference>::const_iterator ai,
			const std::vector<PostingIteratorReference>::const_iterator& ae);
	/// \brief Destructor
	~DocnoAllMatchItr(){}

	/// \brief Find the least upper bound of all non empty matches of docno_
	/// \param[in] docno_ the minimal document number
	/// \return the upper bound or 0 if it does not exist
	Index skipDoc( const Index& docno_);

	/// \brief Find the least upper bound of all candidate matches of docno_
	/// \param[in] docno_ the minimal document number
	/// \return the upper bound or 0 if it does not exist
	Index skipDocCandidate( const Index& docno_);

	/// \brief Get the minimum document frequency (first element df)
	GlobalCounter maxDocumentFrequency() const;
	/// \brief Get the minimum document frequency (last element df)
	GlobalCounter minDocumentFrequency() const;

private:
	std::vector<PostingIteratorReference> m_args;	///< argument posting iterators
	Index m_curdocno;				///< current last docno match
	Index m_curdocno_candidate;			///< current last docno match candidate
};

}//namespace
#endif

