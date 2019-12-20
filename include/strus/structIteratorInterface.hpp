/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for an iterator on structures defined as relations of position info index ranges (directed graph)
/// \file "structIteratorInterface.hpp"
#ifndef _STRUS_STRUCTURE_ITERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_STRUCTURE_ITERATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>

namespace strus
{

/// \class StructIteratorInterface
/// \brief Structure that represents relations of position info index ranges (directed graph) per document.
class StructIteratorInterface
{
public:
	/// \brief Destructor
	virtual ~StructIteratorInterface(){}

	/// \brief Return the next document match with a document number higher than or equal to a given document number
	/// \param[in] docno the minimum document number to fetch
	/// \return a valid local document number or 0
	virtual Index skipDoc( const Index& docno)=0;

	/// \brief Return the first matching source index range with a end position higher than firstpos in the current document or {0,0}. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	/// \param[in] firstpos the minimum position to fetch
	/// \remark The index range is not guaranteed to cover 'firstpos', this has to be checked by the called with IndexRange::start >= firstpos of the returned index range
	/// \note use IndexRange::defined() to check if a matching index range has been found
	/// \return a valid index range or {0,0}
	virtual IndexRange skipPosSource( const Index& firstpos)=0;

	/// \brief Return the first matching sink index range attached to the current source index range with a end position higher than firstpos in the current document or {0,0}. The current source index range is the one returned with the last 'skipPosSource( const Index&)' call.
	/// \param[in] firstpos the minimum position to fetch
	/// \remark The index range is not guaranteed to cover 'firstpos', this has to be checked by the called with IndexRange::start >= firstpos of the returned index range
	/// \note use IndexRange::defined() to check if a matching index range has been found
	/// \return a valid index range or {0,0}
	virtual IndexRange skipPosSink( const Index& firstpos)=0;

	/// \brief Return the last source index range retrieved with 'skipPosSource( const Index&)' without having called 'skipDoc( const Index&)' thereafter.
	/// \return a valid index range or {0,0}
	virtual IndexRange source() const=0;

	/// \brief Return the last source index range retrieved with 'skipPosSink( const Index&)' without having called 'skipPosSource( const Index&)' or 'skipDoc( const Index&)' thereafter.
	/// \return a valid index range or {0,0}
	virtual IndexRange sink() const=0;
};

}//namespace
#endif


