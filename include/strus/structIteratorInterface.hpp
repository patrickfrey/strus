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

	/// \brief Specifies a link associating a field to a structure
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
		const StructureLink& link( int idx) const	{return m_links[idx];}

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

	private:
		StructureLink m_links[ MaxNofLinks];
		int m_nofLinks;
	};

	/// \brief Return the links associated with the last field retrieved with 'skipPos( int, const Index&)' of the specified level without having called 'skipDoc( const Index&)' thereafter.
	/// \return a list of links as structure
	virtual StructureLinkArray links( int level) const=0;
};

}//namespace
#endif


