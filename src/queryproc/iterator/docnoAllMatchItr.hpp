/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Iterator on the all document matches

#ifndef _STRUS_DOCNO_ALL_MATCH_ITR_HPP_INCLUDED
#define _STRUS_DOCNO_ALL_MATCH_ITR_HPP_INCLUDED
#include "strus/index.hpp"
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
	Index maxDocumentFrequency() const;
	/// \brief Get the minimum document frequency (last element df)
	Index minDocumentFrequency() const;

private:
	std::vector<PostingIteratorReference> m_args;	///< argument posting iterators
	Index m_curdocno;				///< current last docno match
	Index m_curdocno_candidate;			///< current last docno match candidate
};

}//namespace
#endif

