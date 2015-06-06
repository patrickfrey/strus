/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Interface for allocating ranges of document numbers in the storage to avoid synchronization and for compaction of certain features like user rights when inserting documents with similar properties in a more natural order.
/// \file "docnoRangeAllocatorInterface.hpp"
#ifndef _STRUS_DOCNO_RANGE_ALLOCATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_DOCNO_RANGE_ALLOCATOR_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"

namespace strus {

/// \class DocnoRangeAllocatorInterface
/// \brief Interface for allocating ranges of document numbers
/// \note Needed for optimization of insertion process and to reduce the complexity of ACL (access control list) representation
class DocnoRangeAllocatorInterface
{
public:
	/// \brief Destructor
	virtual ~DocnoRangeAllocatorInterface(){}

	/// \brief Allocate a set of document numbers
	/// \param[in] size number of document numbers to allocate
	/// \return First document number of allocated document number range
	virtual Index allocDocnoRange( const Index& size)=0;

	/// \brief Deallocate a set of document numbers (if possible)
	/// \param[in] docno first document number of document number range to deallocate
	/// \param[in] size number of document numbers to deallocate
	/// \return true, on success, false, if failed
	/// \remark May fail if the deallocation would lead to gaps in the set of available document numbers
	virtual bool deallocDocnoRange( const Index& docno, const Index& size)=0;
};

}//namespace
#endif

