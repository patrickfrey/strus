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
/// \brief Structure that represents relations of position info ordinal position ranges (directed graph) per document.
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

	/// \brief Return the first matching field (ordinal position range) on a defined level with a end position higher than firstpos in the current document or {0,0}. The current document is the one returned with the last 'skipDoc( const Index&)' call.
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

	/// \brief Structure that describes a header field and its level in the hierarchy of inclusion
	/// \note For calculating the real position in the inclusion hierarchy you have to determine if the header is covered by its content and subtract one in this case
	class HeaderField
	{
	public:
		/// \brief Default constructor
		HeaderField()
			:m_field(),m_hierarchy(-1){}
		/// \brief Constructor
		HeaderField( const strus::IndexRange& field_, int hierarchy_)
			:m_field(field_),m_hierarchy(hierarchy_){}
		/// \brief Copy constructor
		HeaderField( const HeaderField& o)
			:m_field(o.m_field),m_hierarchy(o.m_hierarchy){}

		/// \brief Field (ordinal position range)
		const strus::IndexRange& field() const	{return m_field;}
		/// \brief hierarchy index in the inclusion hierarchy
		int hierarchy() const			{return m_hierarchy;}
		/// \brief Evaluate if this is a valid field
		bool defined() const			{return m_hierarchy>=0;}

	private:
		strus::IndexRange m_field;
		int m_hierarchy;
	};

	/// \brief Return the header field of a defined structure
	/// \param[in] structIndex index of the structure defined in a link retrieved with links
	/// \return a valid field with level or {{0,0},-1} if not defined
	virtual HeaderField headerField( int structIndex) const =0;
};

}//namespace
#endif


