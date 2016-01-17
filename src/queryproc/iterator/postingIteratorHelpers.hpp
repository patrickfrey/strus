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
/// \brief Helper functions shared by iterators

#ifndef _STRUS_ITERATOR_HELPERS_HPP_INCLUDED
#define _STRUS_ITERATOR_HELPERS_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <string>

namespace strus
{

Index getFirstAllMatchDocno(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno,
		bool allowEmpty);

Index getFirstAllMatchDocnoSubset(
		std::vector<Reference< PostingIteratorInterface> >& args,
		Index docno,
		bool allowEmpty,
		std::size_t cardinality);

void encodeInteger( std::string& buf, int val);

}//namespace
#endif

