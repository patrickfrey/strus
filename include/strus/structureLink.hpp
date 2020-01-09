/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure to represent structure links than relate two fields to represent a structure
/// \file "structureLink.hpp"
#ifndef _STRUS_STRUCTURE_LINK_HPP_INCLUDED
#define _STRUS_STRUCTURE_LINK_HPP_INCLUDED
#include "strus/index.hpp"
#include <vector>


namespace strus
{

/// \brief Specifies a link associating a field to a structure instance
class StructureLink
{
public:
	/// \brief Evaluate if the link points to a structure header (true) or to a structure content (false)
	bool header() const		{return m_header;}
	/// \brief Get the number of the structure associated with its name
	strus::Index structno() const	{return m_structno;}
	/// \brief Get the index of the structure separating different structures
	int index() const		{return m_index;}
	/// \brief Evaluate if the link is valid
	bool defined() const		{return !!m_structno;}

	/// \brief Default constructor
	StructureLink()
		:m_structno(0),m_header(false),m_index(0){}
	/// \brief Constructor
	StructureLink( strus::Index structno_, bool header_, int index_)
		:m_structno(structno_),m_header(header_),m_index(index_){}
	/// \brief Copy constructor
	StructureLink( const StructureLink& o)
		:m_structno(o.m_structno),m_header(o.m_header),m_index(o.m_index){}
	/// \brief Assignment
	StructureLink& operator=( const StructureLink& o)
		{m_structno=o.m_structno;m_header=o.m_header;m_index=o.m_index;return *this;}
	void init( strus::Index structno_, bool header_, int index_)
		{m_structno=structno_;m_header=header_;m_index=index_;}

private:
	strus::Index m_structno;
	bool m_header;
	int m_index;
};


/// \brief Defines a static array of links associated with a field
class StructureLinkArray
{
public:
	enum {MaxNofLinks=4};

	/// \brief Get the number of links defined {0..MaxNofLinks-1}
	int nofLinks() const				{return m_nofLinks;}
	/// \brief Get the link addressed by index {0..MaxNofLinks-1}
	const StructureLink& operator[]( int idx) const	{return m_links[idx];}

	/// \brief Default constructor
	StructureLinkArray()
		:m_nofLinks(0){}
	/// \brief Constructor
	StructureLinkArray( StructureLink* links_, int size_)
		{for (int ai=0; ai < size_; ++ai) {m_links[ ai] = links_[ ai];}}
	/// \brief Copy constructor
	StructureLinkArray( const StructureLinkArray& o)
		:m_nofLinks(o.m_nofLinks){for (int ai=0; ai < o.m_nofLinks; ++ai) {m_links[ ai] = o.m_links[ ai];}}
	/// \brief Assignment
	StructureLinkArray& operator=( const StructureLinkArray& o)
		{m_nofLinks=o.m_nofLinks; for (int ai=0; ai < o.m_nofLinks; ++ai) {m_links[ ai] = o.m_links[ ai];}; return *this;}
	void init( StructureLink* links_, int size_)
		{m_nofLinks=size_; for (int ai=0; ai < size_; ++ai) {m_links[ ai] = links_[ ai];}}
	void reset()
		{m_nofLinks=0;}

private:
	StructureLink m_links[ MaxNofLinks];
	int m_nofLinks;
};

}//namespace
#endif


