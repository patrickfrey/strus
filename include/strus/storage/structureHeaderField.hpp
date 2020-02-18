/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure that describes a header field and its level in the hierarchy of inclusion
/// \file "structureHeaderField.hpp"
#ifndef _STRUS_STRUCTURE_HEADER_FIELD_HPP_INCLUDED
#define _STRUS_STRUCTURE_HEADER_FIELD_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <vector>

namespace strus
{

/// \brief Structure that describes a header field and its level in the hierarchy of inclusion
/// \note For calculating the real position in the inclusion hierarchy you have to determine if the header is covered by its content and subtract one in this case
class StructureHeaderField
{
public:
	/// \brief Default constructor
	StructureHeaderField()
		:m_field(),m_hierarchy(-1){}
	/// \brief Constructor
	StructureHeaderField( const strus::IndexRange& field_, int hierarchy_)
		:m_field(field_),m_hierarchy(hierarchy_){}
	/// \brief Copy constructor
	StructureHeaderField( const StructureHeaderField& o)
		:m_field(o.m_field),m_hierarchy(o.m_hierarchy){}

	/// \brief Field (ordinal position range)
	const strus::IndexRange& field() const	{return m_field;}
	/// \brief hierarchy index in the inclusion hierarchy
	int hierarchy() const			{return m_hierarchy;}
	/// \brief Evaluate if this is a valid field
	bool defined() const			{return m_hierarchy>=0;}

	/// \brief Comparison for sorting
	bool operator < (const StructureHeaderField& o) const
	{
		return m_hierarchy == o.m_hierarchy ? m_field < o.m_field : m_hierarchy < o.m_hierarchy;
	}

private:
	strus::IndexRange m_field;
	int m_hierarchy;
};

}//namespace
#endif


