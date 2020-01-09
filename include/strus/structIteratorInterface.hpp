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
#include "strus/structureLink.hpp"
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

	/// \brief Load the structures of the document with a given document number
	/// \param[in] docno the document number to fetch the structures of
	virtual void skipDoc( const Index& docno)=0;

	/// \brief Get the number of levels of structures
	/// \note Levels of structures are created for overlapping structures, a structure of level K is overlapping some structures of level K+1, starting with level 0
	/// \return the number of structure levels in the current document or 0 if undefined
	virtual int levels() const=0;

	/// \brief Get the current document number
	/// \return the current document number or 0 if undefined
	virtual Index docno() const=0;

	/// \brief Return the first matching field (index range) on a defined level with a end position higher than firstpos in the current document or {0,0}. The current document is the one returned with the last 'skipDoc( const Index&)' call.
	/// \param[in] firstpos the minimum position to fetch
	/// \remark The field is not guaranteed to cover 'firstpos', this has to be checked by the called with IndexRange::start >= firstpos of a returned valid field
	/// \note use IndexRange::defined() to check if a matching field has been found and is valid
	/// \return a valid field or {0,0}
	virtual IndexRange skipPos( int level, const Index& firstpos)=0;

	/// \brief Return the last field retrieved with 'skipPos( int, const Index&)' without having called 'skipDoc( const Index&)' thereafter.
	/// \return a valid field or {0,0}
	virtual IndexRange field( int level) const=0;

	/// \brief Return the links associated with the last field retrieved with 'skipPos( int, const Index&)' of the specified level without having called 'skipDoc( const Index&)' thereafter.
	/// \return a list of links as structure
	virtual StructureLinkArray links( int level) const=0;
};

}//namespace
#endif


