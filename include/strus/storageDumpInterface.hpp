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
/// \brief Interface for dumping the contents of the storage
/// \file storageDumpInterface.hpp
#ifndef _STRUS_STORAGE_DUMP_INTERFACE_HPP_INCLUDED
#define _STRUS_STORAGE_DUMP_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Interface for fetching the dump of a strus IR storage
class StorageDumpInterface
{
public:
	/// \brief Destructor
	virtual ~StorageDumpInterface(){}

	/// \brief Fetch the next chunk of the dump
	/// \param[out] chunk pointer to the chunk
	/// \param[out] chunksize size of the chunk in bytes
	/// \return true, if there are more chunks left to return, false if EOF has been reached and no chunk is returned
	virtual bool nextChunk( const char*& chunk, std::size_t& chunksize)=0;
};
}
#endif

