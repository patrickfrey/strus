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
#ifndef _STRUS_QUERY_PARSER_SET_DIM_DESCRIPTION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_SET_DIM_DESCRIPTION_HPP_INCLUDED
#include <cstdlib>

#error DEPRECATED

namespace strus {
namespace parser {

/// \brief Attributes of a set needed in the initial parsing phase
struct SetDimDescription
{
	SetDimDescription()
		:id(0),nofElements(0){}
	SetDimDescription( const SetDimDescription& o)
		:id(o.id),nofElements(o.nofElements){}
	SetDimDescription( std::size_t id_, std::size_t nofElements_)
		:id(id_),nofElements(nofElements_){}
	
	std::size_t id;			///< internal identifier of the set
	std::size_t nofElements;	///< number of elements in the set
};

}}//namespace
#endif
